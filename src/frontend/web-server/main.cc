#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#if defined(_WIN32)
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#include <sys/ioctl.h>
#endif
#if defined(_OPENMP)
#include <omp.h>
#endif

#include <cpp-httplib/httplib.h>

#include <xtcore/xtcore.h>
#include <xtcore/log.h>

#include "job_manager.h"
#include "routes.h"
#include "backend_log.h"
#include "workspace_manager.h"

namespace {

bool arg_eq(const char *arg, const char *name)
{
    return std::strcmp(arg, name) == 0;
}

void print_usage(const char *argv0)
{
    std::printf("Usage: %s [--host <ip>] [--port <num>] [--scene-dir <path>] [--web-root <path>] [--max-concurrent-renders <n>] [--render-reserve-threads <n>] [--verbose]\n", argv0);
}

const char *to_backend_level(xtcore::LOGENTRY_TYPE type)
{
    switch (type) {
    case xtcore::LOGENTRY_DEBUG:
        return "debug";
    case xtcore::LOGENTRY_MESSAGE:
        return "info";
    case xtcore::LOGENTRY_WARNING:
        return "warn";
    case xtcore::LOGENTRY_ERROR:
        return "error";
    }
    return "info";
}

void forward_xtcore_log(xtcore::LOGENTRY_TYPE type, const std::string &message, void *)
{
    xtracer::frontend::web::backend_log_t::handle().add(to_backend_level(type), message);
}

std::string now_verbose_timestamp()
{
    std::time_t t = std::time(nullptr);
    std::tm tm_now;
#if defined(_WIN32)
    localtime_s(&tm_now, &t);
#else
    localtime_r(&t, &tm_now);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

bool stdout_supports_ansi()
{
#if defined(_WIN32)
    return _isatty(_fileno(stdout)) != 0;
#else
    if (isatty(fileno(stdout)) == 0) return false;
    const char *term = std::getenv("TERM");
    if (!term || std::strcmp(term, "dumb") == 0) return false;
    return true;
#endif
}

const char *http_status_color(int status)
{
    if (status >= 200 && status < 300) return "\033[32m"; // green
    if (status >= 300 && status < 400) return "\033[36m"; // cyan
    if (status >= 400 && status < 500) return "\033[33m"; // yellow
    if (status >= 500 && status < 600) return "\033[31m"; // red
    return "\033[37m"; // gray/white
}

long request_duration_ms(const httplib::Request &req)
{
    const std::string raw = req.get_header_value("X-Request-Duration-Ms");
    if (raw.empty()) return -1;
    char *end = nullptr;
    long ms = std::strtol(raw.c_str(), &end, 10);
    if (!end || *end != '\0' || ms < 0) return -1;
    return ms;
}

bool is_safe_client_id_char(unsigned char c)
{
    return (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9')
        || c == '_' || c == '-' || c == '.';
}

std::string request_client_tag(const httplib::Request &req)
{
    auto parse_cookie_client_id = [&req]() -> std::string {
        const std::string cookie = req.get_header_value("Cookie");
        if (cookie.empty()) return "";
        const char *needle = "client_id=";
        const size_t n = std::strlen(needle);
        size_t i = 0;
        while (i < cookie.size()) {
            while (i < cookie.size() && (cookie[i] == ' ' || cookie[i] == ';')) ++i;
            if (i >= cookie.size()) break;
            if (cookie.compare(i, n, needle) == 0) {
                size_t begin = i + n;
                size_t end = begin;
                while (end < cookie.size() && cookie[end] != ';') ++end;
                return cookie.substr(begin, end - begin);
            }
            while (i < cookie.size() && cookie[i] != ';') ++i;
        }
        return "";
    };

    std::string client_id;
    if (req.has_param("client_id")) client_id = req.get_param_value("client_id");
    if (client_id.empty()) client_id = parse_cookie_client_id();
    if (client_id.empty()) return "no-client";
    if (client_id.size() > 96) return "invalid-client";
    for (size_t i = 0; i < client_id.size(); ++i) {
        if (!is_safe_client_id_char((unsigned char)client_id[i])) return "invalid-client";
    }
    return client_id;
}

std::string compact_client_tag(const std::string &tag)
{
    if (tag.size() > 7 && tag.compare(0, 7, "client_") == 0) return tag.substr(7);
    if (tag.size() > 7 && tag.compare(0, 7, "client-") == 0) return tag.substr(7);
    return tag;
}

std::string cancel_job_annotation(const httplib::Request &req)
{
    if (req.method != "POST") return "";
    const std::string prefix = "/api/jobs/abort/";
    if (req.path.size() <= prefix.size()) return "";
    if (req.path.compare(0, prefix.size(), prefix) != 0) return "";
    const std::string id = req.path.substr(prefix.size());
    if (id.empty()) return "";
    return " cancel_job=" + id;
}

int terminal_width_columns()
{
#if defined(_WIN32)
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h == INVALID_HANDLE_VALUE) return 0;
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(h, &info)) return 0;
    const int w = (int)(info.srWindow.Right - info.srWindow.Left + 1);
    return (w > 0) ? w : 0;
#else
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0) return 0;
    const int w = (int)ws.ws_col;
    return (w > 0) ? w : 0;
#endif
}

std::string pad_right(const std::string &s, size_t width)
{
    if (s.size() >= width) return s;
    return s + std::string(width - s.size(), ' ');
}

std::string pad_left(const std::string &s, size_t width)
{
    if (s.size() >= width) return s;
    return std::string(width - s.size(), ' ') + s;
}

std::string fit_field(const std::string &s, size_t width)
{
    if (s.size() == width) return s;
    if (s.size() < width) return pad_right(s, width);
    if (width <= 3) return s.substr(0, width);
    return s.substr(0, width - 3) + "...";
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
    std::printf("\n");
    std::printf("[xtracer_web] startup\n");
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
    std::string web_root = "src/frontend/web-client";
    size_t max_concurrent_renders = 999;
    size_t render_reserve_threads = 1;
    bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        if (arg_eq(argv[i], "--host")) {
            if (++i >= argc) {
                print_usage(argv[0]);
                return 1;
            }
            host = argv[i];
        }
        else if (arg_eq(argv[i], "--port")) {
            if (++i >= argc) {
                print_usage(argv[0]);
                return 1;
            }
            port = std::atoi(argv[i]);
        }
        else if (arg_eq(argv[i], "--scene-dir")) {
            if (++i >= argc) {
                print_usage(argv[0]);
                return 1;
            }
            scene_dir = argv[i];
        }
        else if (arg_eq(argv[i], "--web-root")) {
            if (++i >= argc) {
                print_usage(argv[0]);
                return 1;
            }
            web_root = argv[i];
        }
        else if (arg_eq(argv[i], "--max-concurrent-renders")) {
            if (++i >= argc) {
                print_usage(argv[0]);
                return 1;
            }
            char *end = nullptr;
            unsigned long parsed = std::strtoul(argv[i], &end, 10);
            if (!end || *end != '\0' || parsed == 0) {
                std::printf("Invalid max concurrent renders: %s\n", argv[i]);
                return 1;
            }
            max_concurrent_renders = static_cast<size_t>(parsed);
        }
        else if (arg_eq(argv[i], "--render-reserve-threads")) {
            if (++i >= argc) {
                print_usage(argv[0]);
                return 1;
            }
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
            verbose = true;
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

    xtcore::Log::handle().callback(&forward_xtcore_log, nullptr);
    xtcore::init();
    xtcore::Log::handle().echo(false);
    xtracer::frontend::web::backend_log_t::handle().add("info", "xtracer_web boot");

    httplib::Server server;
    if (verbose) {
        const bool use_ansi = stdout_supports_ansi();
        server.set_logger([use_ansi](const httplib::Request &req, const httplib::Response &res) {
            const long elapsed_ms = request_duration_ms(req);
            const bool slow_exempt = req.path == "/api/logs/wait";
            const bool slow = (elapsed_ms >= 500) && !slow_exempt;
            const char *color = slow ? "\033[31m" : http_status_color(res.status);
            const char *dur_color = slow ? "\033[31m" : "\033[90m";
            const char *reset = "\033[0m";
            char status_buf[16];
            std::snprintf(status_buf, sizeof(status_buf), "%3d", res.status);
            const std::string ts = now_verbose_timestamp();
            const std::string client_tag = compact_client_tag(request_client_tag(req));
            const std::string client_field = fit_field(client_tag, 18);
            const std::string method_field = pad_right(req.method, 6);
            const std::string cancel_note = cancel_job_annotation(req);
            const std::string prefix_plain = "[" + ts + "] [" + client_field + "] "
                + status_buf + " "
                + method_field + " "
                + req.path
                + cancel_note;
            const std::string duration_plain = std::to_string((elapsed_ms >= 0 ? elapsed_ms : 0)) + "ms";
            const std::string duration_field = pad_left(duration_plain, 8);
            const int cols = terminal_width_columns();
            size_t pad_spaces = 1;
            if (cols > 0) {
                const size_t used = prefix_plain.size() + duration_field.size();
                if ((size_t)cols > used + 1) pad_spaces = (size_t)cols - used;
            }
            const std::string gap(pad_spaces, ' ');
            if (use_ansi) {
                std::printf("[%s] [%s] %s%3d%s %-6s %s%s%s%s%s\n",
                            ts.c_str(),
                            client_field.c_str(),
                            color,
                            res.status,
                            reset,
                            req.method.c_str(),
                            (req.path + cancel_note).c_str(),
                            gap.c_str(),
                            dur_color,
                            duration_field.c_str(),
                            reset);
            } else {
                std::printf("[%s] [%s] %3d %-6s %s%s%s\n",
                            ts.c_str(),
                            client_field.c_str(),
                            res.status,
                            req.method.c_str(),
                            (req.path + cancel_note).c_str(),
                            gap.c_str(),
                            duration_field.c_str());
            }
            std::fflush(stdout);
        });
    }
    xtracer::frontend::web::job_manager_t jobs;
    jobs.set_max_concurrent_renders(max_concurrent_renders);
    const size_t render_thread_budget = compute_auto_render_threads(render_reserve_threads);
    jobs.set_render_thread_budget(render_thread_budget);
    xtracer::frontend::web::workspace_manager_t workspaces;
    xtracer::frontend::web::render_thread_policy_t thread_policy;
    thread_policy.reserve_threads = render_reserve_threads;
    thread_policy.max_render_threads = render_thread_budget;
    xtracer::frontend::web::setup_routes(server, jobs, workspaces, scene_dir, web_root, thread_policy);

    print_startup_banner(host, port, scene_dir, web_root, max_concurrent_renders, render_reserve_threads, verbose);

    xtracer::frontend::web::backend_log_t::handle().add(
        "info",
        "listen start host=" + host
        + " port=" + std::to_string(port)
        + " max_concurrent_renders=" + std::to_string(max_concurrent_renders)
        + " render_reserve_threads=" + std::to_string(render_reserve_threads));
    bool ok = server.listen(host.c_str(), port);
    if (!ok) {
        std::printf("Failed to listen on %s:%d\n", host.c_str(), port);
        xtracer::frontend::web::backend_log_t::handle().add("error", "listen failed host=" + host + " port=" + std::to_string(port));
        xtcore::Log::handle().callback(nullptr, nullptr);
        xtcore::deinit();
        return 1;
    }

    xtracer::frontend::web::backend_log_t::handle().add("info", "listen stopped");
    xtcore::Log::handle().callback(nullptr, nullptr);
    xtcore::deinit();
    return 0;
}
