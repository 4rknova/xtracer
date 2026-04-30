#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#if defined(_OPENMP)
#include <omp.h>
#endif

#include <crow_all.h>

#include <xtcore/xtcore.h>
#include <xtcore/log.h>

#include "gallery_manager.h"
#include "job_manager.h"
#include "request_logger_middleware.h"
#include "routes.h"
#include "backend_log.h"
#include "workspace_manager.h"

// Defined here; declared extern in request_logger_middleware.h
bool g_web_verbose = false;

// Custom Crow log handler: suppresses expected ECONNRESET/EPIPE noise that
// Crow emits when a client disconnects abruptly mid-send.  These are normal
// TCP events (browser tab closed, network drop) and do not indicate bugs.
// All other messages pass through to Crow's default stderr handler.
namespace {
class filtered_log_handler_t : public crow::ILogHandler
{
public:
    void log(std::string message, crow::LogLevel level) override
    {
        if (level == crow::LogLevel::Error) {
            // "happened while sending buffers" — from do_write_sync on ECONNRESET.
            // "Connection reset by peer" / "Broken pipe" — async send to dead socket.
            if (message.find("happened while sending buffers") != std::string::npos) return;
            if (message.find("Connection reset by peer")       != std::string::npos) return;
            if (message.find("Broken pipe")                    != std::string::npos) return;
        }
        default_.log(std::move(message), level);
    }
private:
    crow::CerrLogHandler default_;
};
} // namespace

namespace {

bool arg_eq(const char *arg, const char *name)
{
    return std::strcmp(arg, name) == 0;
}

void print_usage(const char *argv0)
{
    std::printf("Usage: %s [--host <ip>] [--port <num>] [--scene-dir <path>] [--gallery-dir <path>] [--web-root <path>] [--max-concurrent-renders <n>] [--render-reserve-threads <n>] [--verbose]\n", argv0);
}

const char *to_backend_level(xtcore::LOGENTRY_TYPE type)
{
    switch (type) {
    case xtcore::LOGENTRY_DEBUG:   return "debug";
    case xtcore::LOGENTRY_MESSAGE: return "info";
    case xtcore::LOGENTRY_WARNING: return "warn";
    case xtcore::LOGENTRY_ERROR:   return "error";
    }
    return "info";
}

void forward_xtcore_log(xtcore::LOGENTRY_TYPE type, const std::string &message, void *)
{
    xtracer::frontend::web::backend_log_t::handle().add(to_backend_level(type), message);
}

size_t runtime_omp_max_threads()
{
#if defined(_OPENMP)
    int n = omp_get_max_threads();
    return (n > 0) ? static_cast<size_t>(n) : 0;
#else
    return 0;
#endif
}

size_t compute_auto_render_threads(size_t reserve_threads)
{
    const unsigned int logical_cores_raw = std::thread::hardware_concurrency();
    const size_t logical_cores = (logical_cores_raw == 0) ? 1 : static_cast<size_t>(logical_cores_raw);
    const size_t omp_max_threads = runtime_omp_max_threads();
    const size_t capacity = (omp_max_threads > 0) ? omp_max_threads : logical_cores;
    return (capacity > reserve_threads) ? (capacity - reserve_threads) : 1;
}

void print_startup_banner(const std::string &host,
                          int port,
                          const std::string &scene_dir,
                          const std::string &web_root,
                          size_t max_concurrent_renders,
                          size_t render_reserve_threads,
                          bool verbose)
{
    const unsigned int logical_cores_raw = std::thread::hardware_concurrency();
    const size_t logical_cores = (logical_cores_raw == 0) ? 1 : static_cast<size_t>(logical_cores_raw);
    const size_t omp_max_threads = runtime_omp_max_threads();

    std::printf("\n");
    std::printf("██╗  ██╗████████╗██████╗  █████╗  ██████╗███████╗██████╗ \n");
    std::printf("╚██╗██╔╝╚══██╔══╝██╔══██╗██╔══██╗██╔════╝██╔════╝██╔══██╗\n");
    std::printf(" ╚███╔╝    ██║   ██████╔╝███████║██║     █████╗  ██████╔╝\n");
    std::printf(" ██╔██╗    ██║   ██╔══██╗██╔══██║██║     ██╔══╝  ██╔══██╗\n");
    std::printf("██╔╝ ██╗   ██║   ██║  ██║██║  ██║╚██████╗███████╗██║  ██║\n");
    std::printf("╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝╚══════╝╚═╝  ╚═╝\n");
#ifdef NDEBUG
    const char *k_build_type = "release";
#else
    const char *k_build_type = "debug";
#endif
    std::printf("\n");
    std::printf("  version:                %s (%s)\n", xtcore::get_version(), k_build_type);
    std::printf("  url:                    http://%s:%d\n", host.c_str(), port);
    std::printf("  host:                   %s\n", host.c_str());
    std::printf("  port:                   %d\n", port);
    std::printf("  scene-dir:              %s\n", scene_dir.c_str());
    std::printf("  web-root:               %s\n", web_root.c_str());
    std::printf("  max-concurrent-renders: %zu\n", max_concurrent_renders);
    std::printf("  render-reserve-threads: %zu\n", render_reserve_threads);
    std::printf("  logical-cores:          %zu\n", logical_cores);
    if (omp_max_threads > 0) {
        std::printf("  openmp-max-threads:     %zu\n", omp_max_threads);
    } else {
        std::printf("  openmp-max-threads:     n/a\n");
    }
    std::printf("  verbose-http:           %s\n", verbose ? "on" : "off");
    std::printf("\n");
}

} // namespace

int main(int argc, char **argv)
{
    std::string host = "127.0.0.1";
    int port = 8080;
    std::string scene_dir = "scene";
    std::string gallery_dir = "gallery";
    std::string web_root = "src/frontend/web-client";
    size_t max_concurrent_renders = 999;
    size_t render_reserve_threads = 1;

    for (int i = 1; i < argc; ++i) {
        if (arg_eq(argv[i], "--host")) {
            if (++i >= argc) { print_usage(argv[0]); return 1; }
            host = argv[i];
        }
        else if (arg_eq(argv[i], "--port")) {
            if (++i >= argc) { print_usage(argv[0]); return 1; }
            port = std::atoi(argv[i]);
        }
        else if (arg_eq(argv[i], "--scene-dir")) {
            if (++i >= argc) { print_usage(argv[0]); return 1; }
            scene_dir = argv[i];
        }
        else if (arg_eq(argv[i], "--web-root")) {
            if (++i >= argc) { print_usage(argv[0]); return 1; }
            web_root = argv[i];
        }
        else if (arg_eq(argv[i], "--gallery-dir")) {
            if (++i >= argc) { print_usage(argv[0]); return 1; }
            gallery_dir = argv[i];
        }
        else if (arg_eq(argv[i], "--max-concurrent-renders")) {
            if (++i >= argc) { print_usage(argv[0]); return 1; }
            char *end = nullptr;
            unsigned long parsed = std::strtoul(argv[i], &end, 10);
            if (!end || *end != '\0' || parsed == 0) {
                std::printf("Invalid max concurrent renders: %s\n", argv[i]);
                return 1;
            }
            max_concurrent_renders = static_cast<size_t>(parsed);
        }
        else if (arg_eq(argv[i], "--render-reserve-threads")) {
            if (++i >= argc) { print_usage(argv[0]); return 1; }
            char *end = nullptr;
            unsigned long parsed = std::strtoul(argv[i], &end, 10);
            if (!end || *end != '\0') {
                std::printf("Invalid render reserve threads: %s\n", argv[i]);
                return 1;
            }
            render_reserve_threads = static_cast<size_t>(parsed);
        }
        else if (arg_eq(argv[i], "--help")) {
            print_usage(argv[0]);
            return 0;
        }
        else if (arg_eq(argv[i], "--verbose") || arg_eq(argv[i], "-v")) {
            g_web_verbose = true;
        }
        else {
            std::printf("Unknown argument: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    if (port <= 0 || port > 65535) {
        std::printf("Invalid port: %d\n", port);
        return 1;
    }

    xtcore::Log::handle().max_size(1024);
    xtcore::Log::handle().callback(&forward_xtcore_log, nullptr);
    xtcore::init();
    xtcore::Log::handle().echo(false);
    xtracer::frontend::web::backend_log_t::handle().add("info", "xtracer boot");

    xtracer::frontend::web::gallery_manager_t gallery;
    if (!gallery.init(gallery_dir)) {
        std::printf("[warn] gallery directory could not be initialized: %s\n", gallery_dir.c_str());
    } else {
        std::printf("  gallery-dir:            %s\n", gallery_dir.c_str());
    }

    xtracer::frontend::web::job_manager_t jobs;
    jobs.set_gallery_manager(&gallery);
    jobs.set_max_concurrent_renders(max_concurrent_renders);
    const size_t render_thread_budget = compute_auto_render_threads(render_reserve_threads);
    jobs.set_render_thread_budget(render_thread_budget);

    xtracer::frontend::web::workspace_manager_t workspaces;
    xtracer::frontend::web::render_thread_policy_t thread_policy;
    thread_policy.reserve_threads = render_reserve_threads;
    thread_policy.max_render_threads = render_thread_budget;

    WebApp app;
    static filtered_log_handler_t filtered_log_handler;
    crow::logger::setHandler(&filtered_log_handler);
    crow::logger::setLogLevel(crow::LogLevel::Warning);

    xtracer::frontend::web::setup_routes(app, jobs, workspaces, &gallery, scene_dir, web_root, thread_policy);

    print_startup_banner(host, port, scene_dir, web_root, max_concurrent_renders, render_reserve_threads, g_web_verbose);

    xtracer::frontend::web::backend_log_t::handle().add(
        "info",
        "listen start host=" + host
        + " port=" + std::to_string(port)
        + " max_concurrent_renders=" + std::to_string(max_concurrent_renders)
        + " render_reserve_threads=" + std::to_string(render_reserve_threads));

    app.bindaddr(host).port(port).multithreaded().run();

    std::printf("\nShutting down..\n");
    xtracer::frontend::web::backend_log_t::handle().add("info", "listen stopped");

    {
        const size_t active = jobs.get_active_render_count();
        if (active > 0) {
            std::printf("Stopping %zu active render(s)..\n", active);
        }
    }
    jobs.shutdown();
    std::printf("Shutdown complete..\n");

    xtcore::Log::handle().callback(nullptr, nullptr);
    xtcore::deinit();
    return 0;
}
