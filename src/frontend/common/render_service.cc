#include "render_service.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>
#include <xtcore/strpool.h>
#include <xtcore/parseutil.h>
#include <xtcore/integrator.h>
#include <nimg/img.h>

namespace xtracer {
namespace frontend {
namespace common {

namespace {

bool parse_u64_text(const std::string &s, size_t &out)
{
    if (s.empty()) return false;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] < '0' || s[i] > '9') return false;
    }
    unsigned long long v = std::strtoull(s.c_str(), nullptr, 10);
    out = (size_t)v;
    return true;
}

bool parse_f64_text(const std::string &s, double &out)
{
    if (s.empty()) return false;
    char *end = nullptr;
    out = std::strtod(s.c_str(), &end);
    if (end == s.c_str() || !end || *end != '\0') return false;
    return true;
}

struct progress_handler_t : public xtcore::render::tile_event_handler_t
{
    std::atomic<size_t> done;
    size_t total;
    progress_callback_t cb;

    explicit progress_handler_t(size_t tile_count, progress_callback_t callback)
        : done(0), total(tile_count), cb(callback)
    {}

    void handle_event(xtcore::render::tile_t *tile)
    {
        size_t now = ++done;
        if (cb) cb(now, total, tile);
    }
};

static const integrator_control_info_t k_no_controls[] = {};

static const integrator_control_option_t k_debug_mode_options[] = {
      { "depth", "Depth" }
    , { "stencil", "Stencil" }
    , { "normal", "Normal" }
    , { "uv", "UV" }
    , { "emission", "Emission" }
};

static const integrator_control_option_t k_depth_encoding_options[] = {
      { "legacy", "Legacy (1/log(z))" }
    , { "linear", "Linear" }
    , { "log", "Logarithmic" }
    , { "inverse", "Inverse (1/(1+z))" }
};

static const integrator_control_info_t k_debug_views_controls[] = {
      { "mode", "Mode", "enum", "Debug output mode", "normal", nullptr, nullptr, nullptr, nullptr, nullptr, k_debug_mode_options, sizeof(k_debug_mode_options) / sizeof(k_debug_mode_options[0]) }
    , { "depth_encoding", "Depth Encoding", "enum", "Depth output encoding", "legacy", nullptr, nullptr, nullptr, "mode", "depth", k_depth_encoding_options, sizeof(k_depth_encoding_options) / sizeof(k_depth_encoding_options[0]) }
    , { "max_distance", "Max Distance", "float", "Used by depth mode", "1000", "0.01", "1000000", "0.01", "mode", "depth", nullptr, 0 }
};

static const integrator_control_info_t k_ao_controls[] = {
      { "max_distance", "Max Distance", "float", "Maximum AO ray distance", "100", "0.1", "1000", "0.1", nullptr, nullptr, nullptr, 0 }
};

static const integrator_control_info_t k_photon_mapping_controls[] = {
      { "emit_photons", "Emit Photons", "int", "Photon emission count", "20000", "1000", "500000", "1000", nullptr, nullptr, nullptr, 0 }
    , { "gather_radius", "Gather Radius", "float", "Radius used for radiance estimate", "0.25", "0.001", "100", "0.001", nullptr, nullptr, nullptr, 0 }
    , { "gather_k", "Gather K", "int", "Maximum photons to gather", "64", "1", "1024", "1", nullptr, nullptr, nullptr, 0 }
};

static const integrator_info_t k_integrators[] = {
      { "raytracer", "Raytracer (Whitted)", k_no_controls, 0 }
    , { "pathtracer_is", "Pathtracer (IS)", k_no_controls, 0 }
    , { "pathtracer", "Pathtracer (Brute Force)", k_no_controls, 0 }
    , { "photon_mapping", "Photon Mapping", k_photon_mapping_controls, sizeof(k_photon_mapping_controls) / sizeof(k_photon_mapping_controls[0]) }
    , { "debug_views", "Debug Views", k_debug_views_controls, sizeof(k_debug_views_controls) / sizeof(k_debug_views_controls[0]) }
    , { "ao"        , "Ambient Occlusion", k_ao_controls, sizeof(k_ao_controls) / sizeof(k_ao_controls[0]) }
};

std::unique_ptr<xtcore::render::IIntegrator> create_integrator(const std::string &name)
{
    if      (name == "raytracer") return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::raytracer::Integrator());
    else if (name == "pathtracer") return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::pathtracer::Integrator());
    else if (name == "pathtracer_is") return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::pathtracer_is::Integrator());
    else if (name == "photon_mapping") return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::photon_mapping::Integrator());
    else if (name == "debug_views") return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::debug_views::Integrator());
    else if (name == "depth")      return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::debug_views::Integrator(xtcore::integrator::debug_views::Integrator::VIEW_DEPTH));
    else if (name == "stencil")    return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::debug_views::Integrator(xtcore::integrator::debug_views::Integrator::VIEW_STENCIL));
    else if (name == "normal")     return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::debug_views::Integrator(xtcore::integrator::debug_views::Integrator::VIEW_NORMAL));
    else if (name == "uv")         return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::debug_views::Integrator(xtcore::integrator::debug_views::Integrator::VIEW_UV));
    else if (name == "emission")   return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::debug_views::Integrator(xtcore::integrator::debug_views::Integrator::VIEW_EMISSION));
    else if (name == "ao")         return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::ao::Integrator());
    return std::unique_ptr<xtcore::render::IIntegrator>();
}

bool read_file_bytes(const char *path, std::vector<unsigned char> &out)
{
    std::ifstream in(path, std::ios::binary);
    if (!in.good()) return false;
    out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return true;
}

bool encode_png_memory(nimg::Pixmap &pixmap, std::vector<unsigned char> &out, std::string &error)
{
#ifdef __EMSCRIPTEN__
    const char *tmp_path = "/tmp/xtracer_wasm.png";
    mkdir("/tmp", 0777);

    int png_err = nimg::io::save::png(tmp_path, pixmap);
    if (png_err != 0) {
        error = "failed to encode png";
        return false;
    }

    bool ok = read_file_bytes(tmp_path, out);
    unlink(tmp_path);
    if (!ok) {
        error = "failed to read generated png";
        return false;
    }
    return true;
#else
    char tmp_path[] = "/tmp/xtracer_web_png_XXXXXX";
    int fd = mkstemp(tmp_path);
    if (fd < 0) {
        error = "failed to create temporary file";
        return false;
    }
    close(fd);

    int png_err = nimg::io::save::png(tmp_path, pixmap);
    if (png_err != 0) {
        unlink(tmp_path);
        error = "failed to encode png";
        return false;
    }

    bool ok = read_file_bytes(tmp_path, out);
    unlink(tmp_path);
    if (!ok) {
        error = "failed to read generated png";
        return false;
    }

    return true;
#endif
}

} // namespace

render_request_t::render_request_t()
    : scene_path()
    , integrator("pathtracer_is")
    , camera()
    , width(640)
    , height(480)
    , threads(0)
    , samples(1)
    , aa(1)
    , rdepth(3)
    , tile_size(32)
    , tile_order(xtcore::render::TILE_ORDER_RANDOM)
{}

render_result_t::render_result_t()
    : ok(false)
    , error()
    , image_png()
    , tiles_done(0)
    , tiles_total(0)
    , elapsed_ms(0.0)
{}

std::vector<integrator_info_t> list_integrators()
{
    return std::vector<integrator_info_t>(k_integrators, k_integrators + sizeof(k_integrators) / sizeof(k_integrators[0]));
}

bool is_integrator_supported(const std::string &name)
{
    return create_integrator(name).get() != nullptr;
}

const integrator_info_t *find_integrator_info(const std::string &name)
{
    for (size_t i = 0; i < sizeof(k_integrators) / sizeof(k_integrators[0]); ++i) {
        if (name == k_integrators[i].id) return &k_integrators[i];
    }
    return nullptr;
}

bool validate_integrator_options(const std::string &integrator,
                                 const std::map<std::string, std::string> &options,
                                 std::string &error)
{
    const integrator_info_t *info = find_integrator_info(integrator);
    if (!info) {
        error = "integrator not supported";
        return false;
    }

    for (auto it = options.begin(); it != options.end(); ++it) {
        const std::string &key = it->first;
        const std::string &value = it->second;

        const integrator_control_info_t *control = nullptr;
        for (size_t i = 0; i < info->controls_count; ++i) {
            if (key == info->controls[i].id) {
                control = &(info->controls[i]);
                break;
            }
        }

        if (!control) {
            error = "unsupported integrator option: " + key;
            return false;
        }

        const std::string type = control->type ? control->type : "";
        if (type == "int") {
            size_t iv = 0;
            if (!parse_u64_text(value, iv)) {
                error = "invalid integer integrator option: " + key;
                return false;
            }
            if (control->min_value && *(control->min_value)) {
                size_t min_v = 0;
                if (parse_u64_text(control->min_value, min_v) && iv < min_v) {
                    error = "integrator option below minimum: " + key;
                    return false;
                }
            }
            if (control->max_value && *(control->max_value)) {
                size_t max_v = 0;
                if (parse_u64_text(control->max_value, max_v) && iv > max_v) {
                    error = "integrator option above maximum: " + key;
                    return false;
                }
            }
        } else if (type == "float") {
            double fv = 0.0;
            if (!parse_f64_text(value, fv)) {
                error = "invalid numeric integrator option: " + key;
                return false;
            }
            if (control->min_value && *(control->min_value)) {
                double min_v = 0.0;
                if (parse_f64_text(control->min_value, min_v) && fv < min_v) {
                    error = "integrator option below minimum: " + key;
                    return false;
                }
            }
            if (control->max_value && *(control->max_value)) {
                double max_v = 0.0;
                if (parse_f64_text(control->max_value, max_v) && fv > max_v) {
                    error = "integrator option above maximum: " + key;
                    return false;
                }
            }
        } else if (type == "bool") {
            if (value != "1" && value != "0" && value != "true" && value != "false") {
                error = "invalid boolean integrator option: " + key;
                return false;
            }
        } else if (type == "enum") {
            bool ok = false;
            for (size_t i = 0; i < control->options_count; ++i) {
                if (value == control->options[i].value) {
                    ok = true;
                    break;
                }
            }
            if (!ok) {
                error = "invalid enum integrator option: " + key;
                return false;
            }
        } else {
            error = "unknown integrator option type: " + key;
            return false;
        }
    }

    return true;
}

render_result_t render_scene_to_png(const render_request_t &request, progress_callback_t on_progress)
{
    render_result_t result;
    xtcore::render::context_t context;

    if (request.scene_path.empty()) {
        result.error = "scene path is empty";
        return result;
    }

    int load_err = xtcore::io::scn::load(&(context.scene), request.scene_path.c_str(), nullptr);
    if (load_err) {
        result.error = "failed to load scene";
        return result;
    }

    if (!request.camera.empty()) {
        context.params.camera = xtcore::pool::str::add(request.camera.c_str());
    } else {
        auto first_cam = context.scene.m_cameras.begin();
        if (first_cam != context.scene.m_cameras.end()) {
            context.params.camera = (*first_cam).first;
        } else {
            context.params.camera = HASH_ID_INVALID;
        }
    }

    if (context.params.camera == HASH_ID_INVALID || !context.scene.get_camera(context.params.camera)) {
        result.error = "no valid camera found";
        return result;
    }

    context.params.width = request.width;
    context.params.height = request.height;
    context.params.threads = request.threads;
    context.params.samples = request.samples;
    context.params.aa = request.aa;
    context.params.rdepth = request.rdepth;
    context.params.tile_size = request.tile_size;
    context.params.tile_order = request.tile_order;
    context.init();

    std::unique_ptr<xtcore::render::IIntegrator> integrator = create_integrator(request.integrator);
    if (!integrator) {
        result.error = "integrator not supported";
        return result;
    }

    result.tiles_total = context.tiles.size();
    progress_handler_t handler(result.tiles_total, on_progress);
    for (auto &tile : context.tiles) {
        tile.setup_handler_on_done(&handler);
    }

    integrator->setup(context);
    integrator->configure(request.integrator_options);
    xtcore::render::order(context.tiles, context.params.tile_order);

    auto t0 = std::chrono::steady_clock::now();
    integrator->render();
    auto t1 = std::chrono::steady_clock::now();
    result.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    nimg::Pixmap framebuffer;
    xtcore::render::assemble(framebuffer, context);
    if (!encode_png_memory(framebuffer, result.image_png, result.error)) {
        return result;
    }

    result.tiles_done = result.tiles_total;
    result.ok = true;
    return result;
}

} /* namespace common */
} /* namespace frontend */
} /* namespace xtracer */
