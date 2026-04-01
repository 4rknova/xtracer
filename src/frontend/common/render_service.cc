#include "render_service.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <mutex>
#include <memory>
#include <map>
#include <sys/stat.h>
#include <unistd.h>
#include <xtcore/strpool.h>
#include <xtcore/parseutil.h>
#include <xtcore/integrator.h>
#include <xtcore/camera.h>
#include <xtcore/tonemapping/tonemapping.h>
#include <nimg/img.h>
#if defined(_OPENMP)
#include <omp.h>
#endif

namespace xtracer {
namespace frontend {
namespace common {

namespace {

struct openmp_thread_limit_guard_t
{
    bool active;
    int previous_threads;

    explicit openmp_thread_limit_guard_t(size_t requested_threads)
        : active(false)
        , previous_threads(0)
    {
#if defined(_OPENMP)
        if (requested_threads == 0) return;
        previous_threads = omp_get_max_threads();
        int limited = (int)requested_threads;
        if (limited < 1) limited = 1;
        omp_set_num_threads(limited);
        active = true;
#else
        (void)requested_threads;
#endif
    }

    ~openmp_thread_limit_guard_t()
    {
#if defined(_OPENMP)
        if (!active) return;
        if (previous_threads > 0) omp_set_num_threads(previous_threads);
#endif
    }
};

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

HASH_ID find_camera_id_by_name(const xtcore::Scene &scene, const std::string &name)
{
    if (name.empty()) return HASH_ID_INVALID;
    for (auto it = scene.m_cameras.begin(); it != scene.m_cameras.end(); ++it) {
        const char *camera_name = xtcore::pool::str::get((*it).first);
        if (camera_name && name == camera_name) return (*it).first;
    }
    return HASH_ID_INVALID;
}

struct progress_state_t
{
    std::atomic<size_t> done;
    size_t total;
    progress_callback_t cb;

    explicit progress_state_t(size_t tile_count, progress_callback_t callback)
        : done(0), total(tile_count), cb(callback)
    {}
};

struct progress_handler_on_init_t : public xtcore::render::tile_event_handler_t
{
    progress_state_t *state;

    explicit progress_handler_on_init_t(progress_state_t *s)
        : state(s)
    {}

    void handle_event(xtcore::render::tile_t *tile)
    {
        if (!state || !state->cb) return;
        state->cb(PROGRESS_EVENT_TILE_STARTED, state->done.load(), state->total, tile, nullptr);
    }
};

struct progress_handler_on_done_t : public xtcore::render::tile_event_handler_t
{
    progress_state_t *state;

    explicit progress_handler_on_done_t(progress_state_t *s)
        : state(s)
    {}

    void handle_event(xtcore::render::tile_t *tile)
    {
        if (!state) return;
        size_t now = ++(state->done);
        if (state->cb) state->cb(PROGRESS_EVENT_TILE_FINISHED, now, state->total, tile, nullptr);
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
      { "emit_photons", "Emit Photons", "int", "Photon emission count", "20000", "1000", "10000000", "1000", nullptr, nullptr, nullptr, 0 }
    , { "caustic_emit_photons", "Caustic Emit", "int", "Caustic photon emission count", "20000", "1000", "10000000", "1000", nullptr, nullptr, nullptr, 0 }
    , { "gather_radius", "Gather Radius", "float", "Radius used for radiance estimate", "0.25", "0.001", "100", "0.001", nullptr, nullptr, nullptr, 0 }
    , { "caustic_gather_radius", "Caustic Radius", "float", "Radius used for caustic estimate", "0.125", "0.0001", "100", "0.0001", nullptr, nullptr, nullptr, 0 }
    , { "gather_k", "Gather K", "int", "Maximum photons to gather", "64", "1", "1024", "1", nullptr, nullptr, nullptr, 0 }
    , { "caustic_gather_k", "Caustic Gather K", "int", "Maximum caustic photons to gather", "32", "1", "2048", "1", nullptr, nullptr, nullptr, 0 }
};

static const integrator_info_t k_integrators[] = {
      { xtcore::render::integrator_metadata_t(), k_no_controls, 0 }
    , { xtcore::render::integrator_metadata_t(), k_no_controls, 0 }
    , { xtcore::render::integrator_metadata_t(), k_no_controls, 0 }
    , { xtcore::render::integrator_metadata_t(), k_no_controls, 0 }
    , { xtcore::render::integrator_metadata_t(), k_photon_mapping_controls, sizeof(k_photon_mapping_controls) / sizeof(k_photon_mapping_controls[0]) }
    , { xtcore::render::integrator_metadata_t(), k_debug_views_controls, sizeof(k_debug_views_controls) / sizeof(k_debug_views_controls[0]) }
    , { xtcore::render::integrator_metadata_t(), k_ao_controls, sizeof(k_ao_controls) / sizeof(k_ao_controls[0]) }
};

static const char *k_integrator_ids[] = {
      "raytracer"
    , "pathtracer_mis"
    , "pathtracer_mis_full"
    , "pathtracer"
    , "photon_mapping"
    , "debug_views"
    , "ao"
};

std::unique_ptr<xtcore::render::IIntegrator> create_integrator(const std::string &name);

const std::vector<integrator_info_t> &integrator_catalog()
{
    static const std::vector<integrator_info_t> catalog = []() {
        std::vector<integrator_info_t> out(k_integrators, k_integrators + sizeof(k_integrators) / sizeof(k_integrators[0]));
        for (size_t i = 0; i < out.size(); ++i) {
            std::unique_ptr<xtcore::render::IIntegrator> integrator = create_integrator(k_integrator_ids[i]);
            if (integrator) out[i].metadata = integrator->metadata();
            else out[i].metadata.id = k_integrator_ids[i];
        }
        return out;
    }();
    return catalog;
}

std::unique_ptr<xtcore::render::IIntegrator> create_integrator(const std::string &name)
{
    if      (name == "raytracer") return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::raytracer::Integrator());
    else if (name == "pathtracer") return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::pathtracer::Integrator());
    else if (name == "pathtracer_mis") return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::pathtracer_is::Integrator());
    else if (name == "pathtracer_is") return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::pathtracer_is::Integrator());
    else if (name == "pathtracer_mis_full") return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::pathtracer_mis_full::Integrator());
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

bool encode_png_memory(nimg::Pixmap &pixmap, std::vector<unsigned char> &out, std::string &error)
{
    if (nimg::io::save::png_memory(pixmap, out) != 0) {
        error = "failed to encode png";
        return false;
    }
    return true;
}

size_t progressive_pass_sample_step(size_t total_samples)
{
    if (total_samples <= 4) return 1;
    if (total_samples <= 16) return 2;
    if (total_samples <= 64) return 4;
    return 8;
}

} // namespace

render_request_t::render_request_t()
    : scene_path()
    , integrator("pathtracer_mis")
    , camera()
    , variant()
    , camera_override()
    , width(640)
    , height(480)
    , threads(0)
    , samples(1)
    , aa(1)
    , rdepth(15)
    , tile_size(32)
    , sample_distribution(xtcore::antialiasing::SAMPLE_DISTRIBUTION_GRID)
    , tile_order(xtcore::render::TILE_ORDER_RANDOM)
    , render_mode(RENDER_MODE_PROGRESSIVE)
{
    camera_override.enabled = false;
    camera_override.px = 0.0;
    camera_override.py = 0.0;
    camera_override.pz = 0.0;
    camera_override.tx = 0.0;
    camera_override.ty = 0.0;
    camera_override.tz = -1.0;
    camera_override.upx = 0.0;
    camera_override.upy = 1.0;
    camera_override.upz = 0.0;
    camera_override.hfov = 60.0;
}

render_result_t::render_result_t()
    : ok(false)
    , aborted(false)
    , error()
    , framebuffer()
    , image_png()
    , tiles_done(0)
    , tiles_total(0)
    , elapsed_ms(0.0)
{}

std::vector<integrator_info_t> list_integrators()
{
    return integrator_catalog();
}

bool is_integrator_supported(const std::string &name)
{
    return create_integrator(name).get() != nullptr;
}

const integrator_info_t *find_integrator_info(const std::string &name)
{
    const std::vector<integrator_info_t> &catalog = integrator_catalog();
    for (size_t i = 0; i < catalog.size(); ++i) {
        if (catalog[i].metadata.id == name) return &catalog[i];
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

render_result_t render_scene_to_png(const render_request_t &request,
                                    progress_callback_t on_progress,
                                    const std::atomic<bool> *abort_flag)
{
    render_result_t result;
    if (request.render_mode == render_request_t::RENDER_MODE_PROGRESSIVE) {
        const size_t total_samples = (request.samples > 0) ? request.samples : 1;
        const size_t step = progressive_pass_sample_step(total_samples);
        std::vector<size_t> pass_samples;
        pass_samples.push_back(1);
        for (size_t rem = (total_samples > 1 ? (total_samples - 1) : 0); rem > 0;) {
            const size_t chunk = (rem > step) ? step : rem;
            pass_samples.push_back(chunk);
            rem -= chunk;
        }
        if (pass_samples.empty()) pass_samples.push_back(1);

        nimg::Pixmap accum_fb;
        accum_fb.init(request.width, request.height);
        std::vector<float> accum_weight(accum_fb.width() * accum_fb.height(), 0.0f);
        for (size_t y = 0; y < accum_fb.height(); ++y) {
            for (size_t x = 0; x < accum_fb.width(); ++x) {
                accum_fb.pixel(x, y) = nimg::ColorRGBAf(0.f, 0.f, 0.f, 1.f);
            }
        }
        std::mutex accum_mut;
        size_t global_tiles_total = 0;
        size_t global_tiles_done = 0;

        for (size_t pass_index = 0; pass_index < pass_samples.size(); ++pass_index) {
            if (abort_flag && abort_flag->load()) {
                result.aborted = true;
                result.error = "render aborted";
                result.tiles_total = global_tiles_total;
                result.tiles_done = global_tiles_done;
                return result;
            }

            render_request_t pass_request = request;
            pass_request.render_mode = render_request_t::RENDER_MODE_DIRECT;
            pass_request.samples = pass_samples[pass_index];
            const float pass_weight = static_cast<float>(pass_samples[pass_index]);
            render_result_t pass_result = render_scene_to_png(
                pass_request,
                [on_progress, pass_index, pass_weight, &pass_samples, &accum_fb, &accum_weight, &accum_mut](progress_event_t event,
                                                                                                              size_t done,
                                                                                                              size_t total,
                                                                                                              const xtcore::render::tile_t *tile,
                                                                                                              const progress_tile_update_t *) {
                    const size_t pass_count = pass_samples.size();
                    const size_t mapped_total = total * pass_count;
                    const size_t mapped_done = (pass_index * total) + done;
                    if (event == PROGRESS_EVENT_TILE_STARTED) {
                        if (on_progress) on_progress(event, mapped_done, mapped_total, tile, nullptr);
                        return;
                    }
                    if (event != PROGRESS_EVENT_TILE_FINISHED || !tile) return;

                    const size_t x0 = tile->x0();
                    const size_t y0 = tile->y0();
                    const size_t x1 = tile->x1();
                    const size_t y1 = tile->y1();

                    std::lock_guard<std::mutex> lock(accum_mut);
                    nimg::ColorRGBAf sample;
                    for (size_t y = y0; y < y1; ++y) {
                        for (size_t x = x0; x < x1; ++x) {
                            tile->read(x, y, sample);
                            const size_t idx = y * accum_fb.width() + x;
                            const float prev_weight = accum_weight[idx];
                            const float next_weight = prev_weight + pass_weight;
                            if (next_weight <= 0.0f) continue;
                            const nimg::ColorRGBAf prev = accum_fb.pixel(x, y);
                            const float sum_r = (prev.r() * prev_weight) + (sample.r() * pass_weight);
                            const float sum_g = (prev.g() * prev_weight) + (sample.g() * pass_weight);
                            const float sum_b = (prev.b() * prev_weight) + (sample.b() * pass_weight);
                            accum_fb.pixel(x, y) = nimg::ColorRGBAf(
                                sum_r / next_weight,
                                sum_g / next_weight,
                                sum_b / next_weight,
                                1.f);
                            accum_weight[idx] = next_weight;
                        }
                    }

                    if (on_progress) {
                        progress_tile_update_t upd;
                        upd.has_rect = true;
                        upd.x0 = x0;
                        upd.y0 = y0;
                        upd.x1 = x1;
                        upd.y1 = y1;
                        upd.source_fb = &accum_fb;
                        on_progress(PROGRESS_EVENT_TILE_FINISHED, mapped_done, mapped_total, nullptr, &upd);
                    }
                },
                abort_flag);

            result.elapsed_ms += pass_result.elapsed_ms;
            global_tiles_total = pass_result.tiles_total * pass_samples.size();
            global_tiles_done = (pass_index + 1) * pass_result.tiles_total;
            if (global_tiles_done > global_tiles_total) global_tiles_done = global_tiles_total;

            if (pass_result.aborted || (abort_flag && abort_flag->load())) {
                result.aborted = true;
                result.error = "render aborted";
                result.tiles_total = global_tiles_total;
                result.tiles_done = global_tiles_done;
                return result;
            }
            if (!pass_result.ok) {
                result.error = pass_result.error;
                result.tiles_total = global_tiles_total;
                result.tiles_done = global_tiles_done;
                return result;
            }
            if (pass_result.framebuffer.width() == 0 || pass_result.framebuffer.height() == 0) {
                result.error = "empty framebuffer";
                result.tiles_total = global_tiles_total;
                result.tiles_done = global_tiles_done;
                return result;
            }

            if (pass_index + 1 == pass_samples.size()) {
                result.photon_diffuse_points = pass_result.photon_diffuse_points;
                result.photon_caustic_points = pass_result.photon_caustic_points;
            }
        }

        nimg::Pixmap ldr_framebuffer = accum_fb;
        xtcore::tonemapping::apply(ldr_framebuffer);
        if (!encode_png_memory(ldr_framebuffer, result.image_png, result.error)) {
            return result;
        }
        result.framebuffer = std::move(accum_fb);
        result.tiles_total = global_tiles_total;
        result.tiles_done = global_tiles_total;
        result.ok = true;
        return result;
    }

    xtcore::render::context_t context;
    openmp_thread_limit_guard_t thread_limit_guard(request.threads);

    if (request.scene_path.empty()) {
        result.error = "scene path is empty";
        return result;
    }

    const char *variant_name = request.variant.empty() ? nullptr : request.variant.c_str();
    int load_err = xtcore::io::scn::load(&(context.scene), request.scene_path.c_str(), nullptr, variant_name);
    if (load_err) {
        result.error = "failed to load scene";
        return result;
    }

    if (!request.camera.empty()) {
        context.params.camera = xtcore::pool::str::add(request.camera.c_str());
    } else {
        context.params.camera = find_camera_id_by_name(context.scene, context.scene.m_default_camera);
        if (context.params.camera == HASH_ID_INVALID) {
            auto first_cam = context.scene.m_cameras.begin();
            if (first_cam != context.scene.m_cameras.end()) {
                context.params.camera = (*first_cam).first;
            } else {
                context.params.camera = HASH_ID_INVALID;
            }
        }
    }

    if (context.params.camera == HASH_ID_INVALID || !context.scene.get_camera(context.params.camera)) {
        result.error = "no valid camera found";
        return result;
    }
    if (request.camera_override.enabled) {
        xtcore::asset::ICamera *cam = context.scene.get_camera(context.params.camera);
        xtcore::camera::Perspective *pcam = dynamic_cast<xtcore::camera::Perspective *>(cam);
        if (!pcam) {
            result.error = "interactive camera override requires perspective camera";
            return result;
        }
        pcam->position = nmath::Vector3f(
            static_cast<float>(request.camera_override.px),
            static_cast<float>(request.camera_override.py),
            static_cast<float>(request.camera_override.pz));
        pcam->target = nmath::Vector3f(
            static_cast<float>(request.camera_override.tx),
            static_cast<float>(request.camera_override.ty),
            static_cast<float>(request.camera_override.tz));
        pcam->up = nmath::Vector3f(
            static_cast<float>(request.camera_override.upx),
            static_cast<float>(request.camera_override.upy),
            static_cast<float>(request.camera_override.upz));
        if (request.camera_override.hfov > 0.01) {
            pcam->fov = static_cast<float>(request.camera_override.hfov);
        }
    }

    context.params.width = request.width;
    context.params.height = request.height;
    context.params.threads = request.threads;
    context.params.samples = request.samples;
    context.params.aa = request.aa;
    context.params.rdepth = request.rdepth;
    context.params.tile_size = request.tile_size;
    context.params.sample_distribution = request.sample_distribution;
    context.params.tile_order = request.tile_order;
    context.init();

    std::unique_ptr<xtcore::render::IIntegrator> integrator = create_integrator(request.integrator);
    if (!integrator) {
        result.error = "integrator not supported";
        return result;
    }

    result.tiles_total = context.tiles.size();
    progress_state_t progress_state(result.tiles_total, on_progress);
    progress_handler_on_init_t handler_on_init(&progress_state);
    progress_handler_on_done_t handler_on_done(&progress_state);
    for (auto &tile : context.tiles) {
        tile.setup_handler_on_init(&handler_on_init);
        tile.setup_handler_on_done(&handler_on_done);
    }

    integrator->setup(context);
    integrator->configure(request.integrator_options);
    static const std::atomic<bool> never_abort(false);
    if (!abort_flag) abort_flag = &never_abort;
    integrator->set_abort_flag(abort_flag);
    xtcore::render::order(context.tiles, context.params.tile_order);

    auto t0 = std::chrono::steady_clock::now();
    integrator->render();
    auto t1 = std::chrono::steady_clock::now();
    result.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    if (abort_flag->load()) {
        result.tiles_total = context.tiles.size();
        result.tiles_done = progress_state.done.load();
        result.aborted = true;
        result.error = "render aborted";
        return result;
    }

    xtcore::integrator::photon_mapping::Integrator *pm =
        dynamic_cast<xtcore::integrator::photon_mapping::Integrator *>(integrator.get());
    if (pm) {
        const std::vector<nmath::Vector3f> &diffuse = pm->debug_global_points();
        const std::vector<nmath::Vector3f> &caustic = pm->debug_caustic_points();
        result.photon_diffuse_points.reserve(diffuse.size());
        result.photon_caustic_points.reserve(caustic.size());
        for (size_t i = 0; i < diffuse.size(); ++i) {
            common::render_result_t::point3_t p;
            p.x = (float)diffuse[i].x;
            p.y = (float)diffuse[i].y;
            p.z = (float)diffuse[i].z;
            result.photon_diffuse_points.push_back(p);
        }
        for (size_t i = 0; i < caustic.size(); ++i) {
            common::render_result_t::point3_t p;
            p.x = (float)caustic[i].x;
            p.y = (float)caustic[i].y;
            p.z = (float)caustic[i].z;
            result.photon_caustic_points.push_back(p);
        }
    }

    nimg::Pixmap framebuffer;
    xtcore::render::assemble(framebuffer, context);
    nimg::Pixmap ldr_framebuffer = framebuffer;
    xtcore::tonemapping::apply(ldr_framebuffer);
    if (!encode_png_memory(ldr_framebuffer, result.image_png, result.error)) {
        return result;
    }
    result.framebuffer = std::move(framebuffer);

    result.tiles_done = result.tiles_total;
    result.ok = true;
    return result;
}

} /* namespace common */
} /* namespace frontend */
} /* namespace xtracer */
