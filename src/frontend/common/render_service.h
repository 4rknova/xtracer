#ifndef XTRACER_FRONTEND_COMMON_RENDER_SERVICE_H_INCLUDED
#define XTRACER_FRONTEND_COMMON_RENDER_SERVICE_H_INCLUDED

#include <string>
#include <vector>
#include <functional>
#include <map>

#include <xtcore/context.h>

namespace xtracer {
namespace frontend {
namespace common {

struct integrator_control_option_t
{
    const char *value;
    const char *label;
};

struct integrator_control_info_t
{
    const char *id;
    const char *label;
    const char *type;
    const char *description;
    const char *default_value;
    const char *min_value;
    const char *max_value;
    const char *step_value;
    const char *visible_when_id;
    const char *visible_when_value;
    const integrator_control_option_t *options;
    size_t options_count;
};

struct integrator_info_t
{
    const char *id;
    const char *label;
    const integrator_control_info_t *controls;
    size_t controls_count;
};

struct render_request_t
{
    std::string scene_path;
    std::string integrator;
    std::string camera;
    std::map<std::string, std::string> integrator_options;

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
const integrator_info_t *find_integrator_info(const std::string &name);
bool validate_integrator_options(const std::string &integrator,
                                 const std::map<std::string, std::string> &options,
                                 std::string &error);
render_result_t render_scene_to_png(const render_request_t &request, progress_callback_t on_progress);

} /* namespace common */
} /* namespace frontend */
} /* namespace xtracer */

#endif /* XTRACER_FRONTEND_COMMON_RENDER_SERVICE_H_INCLUDED */
