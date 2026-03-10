#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <cpp-httplib/httplib.h>

#include <xtcore/xtcore.h>
#include <xtcore/log.h>

#include "job_manager.h"
#include "routes.h"
#include "backend_log.h"

namespace {

bool arg_eq(const char *arg, const char *name)
{
    return std::strcmp(arg, name) == 0;
}

void print_usage(const char *argv0)
{
    std::printf("Usage: %s [--host <ip>] [--port <num>] [--scene-dir <path>] [--web-root <path>]\n", argv0);
}

} // namespace

int main(int argc, char **argv)
{
    std::string host = "127.0.0.1";
    int port = 8080;
    std::string scene_dir = "scene";
    std::string web_root = "res/web";

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
        else if (arg_eq(argv[i], "--help")) {
            print_usage(argv[0]);
            return 0;
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

    xtcore::init();
    xtcore::Log::handle().echo(false);
    xtracer::frontend::web::backend_log_t::handle().add("info", "xtracer_web boot");

    httplib::Server server;
    xtracer::frontend::web::job_manager_t jobs;
    xtracer::frontend::web::setup_routes(server, jobs, scene_dir, web_root);

    std::printf("xtracer_web listening on http://%s:%d\n", host.c_str(), port);
    std::printf("scene-dir=%s\n", scene_dir.c_str());
    std::printf("web-root=%s\n", web_root.c_str());

    xtracer::frontend::web::backend_log_t::handle().add("info", "listen start host=" + host + " port=" + std::to_string(port));
    bool ok = server.listen(host.c_str(), port);
    if (!ok) {
        std::printf("Failed to listen on %s:%d\n", host.c_str(), port);
        xtracer::frontend::web::backend_log_t::handle().add("error", "listen failed host=" + host + " port=" + std::to_string(port));
        xtcore::deinit();
        return 1;
    }

    xtracer::frontend::web::backend_log_t::handle().add("info", "listen stopped");
    xtcore::deinit();
    return 0;
}
