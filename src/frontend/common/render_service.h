#ifndef XTRACER_FRONTEND_COMMON_RENDER_SERVICE_H_INCLUDED
#define XTRACER_FRONTEND_COMMON_RENDER_SERVICE_H_INCLUDED

#include <string>
#include <vector>
#include <functional>
#include <map>
#include <atomic>

#include <xtcore/context.h>
#include <xtcore/integrator.h>
#include <nimg/pixmap.h>

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
    xtcore::render::integrator_metadata_t metadata;
    const integrator_control_info_t *controls;
    size_t controls_count;
};

struct render_request_t
{
    enum render_mode_t {
        RENDER_MODE_DIRECT = 0,
        RENDER_MODE_PROGRESSIVE,
        RENDER_MODE_INCREMENTAL,
        RENDER_MODE_INTERACTIVE
    };

    struct camera_override_t {
        bool enabled;
        double px;
        double py;
        double pz;
        double tx;
        double ty;
        double tz;
        double upx;
        double upy;
        double upz;
        double hfov;
    };

    std::string scene_path;
    std::string scene_cache_key;
    std::string integrator;
    std::string camera;
    std::string variant;
    camera_override_t camera_override;
    std::map<std::string, std::string> integrator_options;

    size_t width;
    size_t height;
    size_t threads;
    size_t samples;
    size_t aa;
    size_t rdepth;
    size_t tile_size;
    xtcore::antialiasing::SAMPLE_DISTRIBUTION sample_distribution;

    xtcore::render::TILE_ORDER tile_order;
    render_mode_t render_mode;
    bool scene_cache_enabled;
    bool save_to_gallery;

    render_request_t();
};

struct render_result_t
{
    struct point3_t {
        float x;
        float y;
        float z;
    };

    bool ok;
    bool aborted;
    std::string error;
    nimg::Pixmap framebuffer;
    std::vector<unsigned char> image_png;
    std::vector<point3_t> photon_diffuse_points;
    std::vector<point3_t> photon_caustic_points;
    size_t tiles_done;
    size_t tiles_total;
    double elapsed_ms;

    render_result_t();
};

enum progress_event_t
{
    PROGRESS_EVENT_TILE_STARTED = 0,
    PROGRESS_EVENT_TILE_FINISHED,
    PROGRESS_EVENT_PASS_FINISHED  // emitted after each complete pass (progressive/incremental)
                                  // done=pass index (1-based), total=total passes
                                  // upd->source_fb points to the accumulated framebuffer
};

struct progress_tile_update_t
{
    bool has_rect;
    size_t x0;
    size_t y0;
    size_t x1;
    size_t y1;
    nimg::Pixmap *source_fb;

    progress_tile_update_t()
        : has_rect(false)
        , x0(0)
        , y0(0)
        , x1(0)
        , y1(0)
        , source_fb(nullptr)
    {}
};

typedef std::function<void(progress_event_t, size_t, size_t, const xtcore::render::tile_t*, const progress_tile_update_t *)> progress_callback_t;

std::vector<integrator_info_t> list_integrators();
bool is_integrator_supported(const std::string &name);
const integrator_info_t *find_integrator_info(const std::string &name);
bool validate_integrator_options(const std::string &integrator,
                                 const std::map<std::string, std::string> &options,
                                 std::string &error);
render_result_t render_scene_to_png(const render_request_t &request,
                                    progress_callback_t on_progress,
                                    const std::atomic<bool> *abort_flag = nullptr);

} /* namespace common */
} /* namespace frontend */
} /* namespace xtracer */

#endif /* XTRACER_FRONTEND_COMMON_RENDER_SERVICE_H_INCLUDED */
