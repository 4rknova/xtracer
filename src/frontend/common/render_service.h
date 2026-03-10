#ifndef XTRACER_FRONTEND_COMMON_RENDER_SERVICE_H_INCLUDED
#define XTRACER_FRONTEND_COMMON_RENDER_SERVICE_H_INCLUDED

#include <string>
#include <vector>
#include <functional>

#include <xtcore/context.h>

namespace xtracer {
namespace frontend {
namespace common {

struct integrator_info_t
{
    const char *id;
    const char *label;
};

struct render_request_t
{
    std::string scene_path;
    std::string integrator;
    std::string camera;

    size_t width;
    size_t height;
    size_t threads;
    size_t samples;
    size_t aa;
    size_t rdepth;
    size_t tile_size;

    xtcore::render::TILE_ORDER tile_order;

    render_request_t();
};

struct render_result_t
{
    bool ok;
    std::string error;
    std::vector<unsigned char> image_png;
    size_t tiles_done;
    size_t tiles_total;
    double elapsed_ms;

    render_result_t();
};

typedef std::function<void(size_t, size_t, const xtcore::render::tile_t*)> progress_callback_t;

std::vector<integrator_info_t> list_integrators();
bool is_integrator_supported(const std::string &name);
render_result_t render_scene_to_png(const render_request_t &request, progress_callback_t on_progress);

} /* namespace common */
} /* namespace frontend */
} /* namespace xtracer */

#endif /* XTRACER_FRONTEND_COMMON_RENDER_SERVICE_H_INCLUDED */
