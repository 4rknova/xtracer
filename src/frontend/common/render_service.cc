#include "render_service.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <mutex>
#include <memory>
#include <list>
#include <map>
#include <sstream>
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
    , { "object_mask", "Object Mask" }
    , { "environment", "Environment" }
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
    , { "objects", "Objects", "string", "Comma-separated object names to include in the mask", "", nullptr, nullptr, nullptr, "mode", "object_mask", nullptr, 0 }
    , { "ignore_geometry", "Ignore Geometry", "bool", "Sample environment for all rays, ignoring geometry", "false", nullptr, nullptr, nullptr, "mode", "environment", nullptr, 0 }
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
    , { xtcore::render::integrator_metadata_t(), k_photon_mapping_controls, sizeof(k_photon_mapping_controls) / sizeof(k_photon_mapping_controls[0]) }
    , { xtcore::render::integrator_metadata_t(), k_debug_views_controls, sizeof(k_debug_views_controls) / sizeof(k_debug_views_controls[0]) }
    , { xtcore::render::integrator_metadata_t(), k_ao_controls, sizeof(k_ao_controls) / sizeof(k_ao_controls[0]) }
    , { xtcore::render::integrator_metadata_t(), k_no_controls, 0 }
};

static const char *k_integrator_ids[] = {
      "raytracer"
    , "pathtracer_mis"
    , "pathtracer"
    , "photon_mapping"
    , "debug_views"
    , "ao"
    , "pathtracer_bdpt"
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
    else if (name == "pathtracer_mis") return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::pathtracer_mis::Integrator());
    else if (name == "photon_mapping") return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::photon_mapping::Integrator());
    else if (name == "debug_views") return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::debug_views::Integrator());
    else if (name == "depth")      return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::debug_views::Integrator(xtcore::integrator::debug_views::Integrator::VIEW_DEPTH));
    else if (name == "stencil")    return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::debug_views::Integrator(xtcore::integrator::debug_views::Integrator::VIEW_STENCIL));
    else if (name == "normal")     return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::debug_views::Integrator(xtcore::integrator::debug_views::Integrator::VIEW_NORMAL));
    else if (name == "uv")         return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::debug_views::Integrator(xtcore::integrator::debug_views::Integrator::VIEW_UV));
    else if (name == "emission")   return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::debug_views::Integrator(xtcore::integrator::debug_views::Integrator::VIEW_EMISSION));
    else if (name == "ao")         return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::ao::Integrator());
    else if (name == "pathtracer_bdpt") return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::pathtracer_bdpt::Integrator());
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

struct pooled_hash_guard_t
{
    HASH_ID value;

    pooled_hash_guard_t()
        : value(HASH_ID_INVALID)
    {}

    ~pooled_hash_guard_t()
    {
        if (value != HASH_ID_INVALID) xtcore::pool::str::del(value);
    }

    void reset(HASH_ID next)
    {
        if (value != HASH_ID_INVALID) xtcore::pool::str::del(value);
        value = next;
    }
};

struct prepared_render_t
{
    xtcore::render::context_t context;
    pooled_hash_guard_t camera_guard;
};

std::string hex_u64(std::uint64_t value)
{
    std::ostringstream ss;
    ss << std::hex << value;
    return ss.str();
}

bool file_cache_stamp(const std::string &path, std::uint64_t &out)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return false;
    std::uint64_t sec = (st.st_mtime >= 0) ? (std::uint64_t)st.st_mtime : 0ULL;
    std::uint64_t nsec = 0ULL;
#if defined(__linux__)
    nsec = (st.st_mtim.tv_nsec >= 0) ? (std::uint64_t)st.st_mtim.tv_nsec : 0ULL;
#elif defined(__APPLE__)
    nsec = (st.st_mtimespec.tv_nsec >= 0) ? (std::uint64_t)st.st_mtimespec.tv_nsec : 0ULL;
#endif
    const std::uint64_t size = (st.st_size >= 0) ? (std::uint64_t)st.st_size : 0ULL;
    out = ((sec & 0xffffffffULL) << 32) ^ (nsec & 0xffffffffULL) ^ (size * 0x9e3779b97f4a7c15ULL);
    return true;
}

std::string fallback_scene_cache_key(const render_request_t &request)
{
    if (request.scene_path.empty()) return std::string();
    std::uint64_t stamp = 0ULL;
    if (!file_cache_stamp(request.scene_path, stamp)) return std::string();

    std::ostringstream ss;
    ss << "file\n"
       << request.scene_path << "\n"
       << request.variant << "\n"
       << hex_u64(stamp);
    return ss.str();
}

bool resolve_scene_cache_key(const render_request_t &request, std::string &out)
{
    out = request.scene_cache_key;
    if (!out.empty()) return true;
    out = fallback_scene_cache_key(request);
    return !out.empty();
}

bool can_reuse_cached_scene(const render_request_t &request)
{
    if (!request.scene_cache_enabled) return false;
    return true;
}

std::unique_ptr<xtcore::asset::ICamera> clone_override_camera(const xtcore::asset::ICamera *base_camera,
                                                              const render_request_t::camera_override_t &camera_override,
                                                              std::string &error)
{
    if (!base_camera) {
        error = "no valid camera found";
        return std::unique_ptr<xtcore::asset::ICamera>();
    }

    const xtcore::camera::Perspective *perspective = dynamic_cast<const xtcore::camera::Perspective *>(base_camera);
    if (perspective) {
        std::unique_ptr<xtcore::camera::Perspective> clone(new xtcore::camera::Perspective(*perspective));
        clone->position = nmath::Vector3f(
            static_cast<float>(camera_override.px),
            static_cast<float>(camera_override.py),
            static_cast<float>(camera_override.pz));
        clone->target = nmath::Vector3f(
            static_cast<float>(camera_override.tx),
            static_cast<float>(camera_override.ty),
            static_cast<float>(camera_override.tz));
        clone->up = nmath::Vector3f(
            static_cast<float>(camera_override.upx),
            static_cast<float>(camera_override.upy),
            static_cast<float>(camera_override.upz));
        if (camera_override.hfov > 0.01) {
            clone->fov = static_cast<float>(camera_override.hfov);
        }
        return std::unique_ptr<xtcore::asset::ICamera>(clone.release());
    }

    const xtcore::camera::TiltShift *tilt_shift = dynamic_cast<const xtcore::camera::TiltShift *>(base_camera);
    if (tilt_shift) {
        std::unique_ptr<xtcore::camera::TiltShift> clone(new xtcore::camera::TiltShift(*tilt_shift));
        clone->position = nmath::Vector3f(
            static_cast<float>(camera_override.px),
            static_cast<float>(camera_override.py),
            static_cast<float>(camera_override.pz));
        clone->target = nmath::Vector3f(
            static_cast<float>(camera_override.tx),
            static_cast<float>(camera_override.ty),
            static_cast<float>(camera_override.tz));
        clone->up = nmath::Vector3f(
            static_cast<float>(camera_override.upx),
            static_cast<float>(camera_override.upy),
            static_cast<float>(camera_override.upz));
        if (camera_override.hfov > 0.01) {
            clone->fov = static_cast<float>(camera_override.hfov);
        }
        return std::unique_ptr<xtcore::asset::ICamera>(clone.release());
    }

    error = "interactive camera override requires perspective camera";
    return std::unique_ptr<xtcore::asset::ICamera>();
}

class prepared_scene_cache_t
{
    public:
    static prepared_scene_cache_t &handle()
    {
        static prepared_scene_cache_t cache;
        return cache;
    }

    bool checkout(const std::string &key, std::unique_ptr<prepared_render_t> &out)
    {
        if (key.empty()) return false;
        std::lock_guard<std::mutex> lock(mut_);
        auto it = entries_.find(key);
        if (it == entries_.end()) return false;
        out = std::move(it->second.prepared);
        lru_.erase(it->second.lru_it);
        entries_.erase(it);
        return out.get() != nullptr;
    }

    void store(const std::string &key, std::unique_ptr<prepared_render_t> prepared)
    {
        if (key.empty() || !prepared) return;
        std::lock_guard<std::mutex> lock(mut_);
        erase_locked(key);
        lru_.push_front(key);
        cache_entry_t entry;
        entry.prepared = std::move(prepared);
        entry.lru_it = lru_.begin();
        entries_[key] = std::move(entry);
        prune_locked();
    }

    private:
    struct cache_entry_t
    {
        std::unique_ptr<prepared_render_t> prepared;
        std::list<std::string>::iterator lru_it;
    };

    prepared_scene_cache_t()
        : mut_()
        , entries_()
        , lru_()
        , max_entries_(4)
    {}

    void erase_locked(const std::string &key)
    {
        auto it = entries_.find(key);
        if (it == entries_.end()) return;
        lru_.erase(it->second.lru_it);
        entries_.erase(it);
    }

    void prune_locked()
    {
        while (entries_.size() > max_entries_ && !lru_.empty()) {
            const std::string key = lru_.back();
            erase_locked(key);
        }
    }

    std::mutex mut_;
    std::map<std::string, cache_entry_t> entries_;
    std::list<std::string> lru_;
    size_t max_entries_;
};

struct prepared_render_lease_t
{
    bool cacheable;
    std::string cache_key;
    std::unique_ptr<prepared_render_t> prepared;

    prepared_render_lease_t()
        : cacheable(false)
        , cache_key()
        , prepared()
    {}

    ~prepared_render_lease_t()
    {
        if (!cacheable || !prepared) return;
        prepared_scene_cache_t::handle().store(cache_key, std::move(prepared));
    }
};

bool load_scene_into_prepared(const render_request_t &request,
                              prepared_render_t &prepared,
                              std::string &error)
{
    if (request.scene_path.empty()) {
        error = "scene path is empty";
        return false;
    }

    const char *variant_name = request.variant.empty() ? nullptr : request.variant.c_str();
    int load_err = xtcore::io::scn::load(&(prepared.context.scene), request.scene_path.c_str(), nullptr, variant_name);
    if (load_err) {
        error = "failed to load scene";
        return false;
    }
    return true;
}

bool configure_prepared_render(const render_request_t &request,
                               prepared_render_t &prepared,
                               std::string &error)
{
    prepared.camera_guard.reset(HASH_ID_INVALID);
    prepared.context.transient_camera.reset();
    if (!request.camera.empty()) {
        prepared.camera_guard.reset(xtcore::pool::str::add(request.camera.c_str()));
        prepared.context.params.camera = prepared.camera_guard.value;
    } else {
        prepared.context.params.camera = find_camera_id_by_name(prepared.context.scene, prepared.context.scene.m_default_camera);
        if (prepared.context.params.camera == HASH_ID_INVALID) {
            auto first_cam = prepared.context.scene.m_cameras.begin();
            if (first_cam != prepared.context.scene.m_cameras.end()) {
                prepared.context.params.camera = (*first_cam).first;
            } else {
                prepared.context.params.camera = HASH_ID_INVALID;
            }
        }
    }

    if (prepared.context.params.camera == HASH_ID_INVALID ||
        !prepared.context.scene.get_camera(prepared.context.params.camera)) {
        error = "no valid camera found";
        return false;
    }

    if (request.camera_override.enabled) {
        prepared.context.transient_camera = clone_override_camera(
            prepared.context.scene.get_camera(prepared.context.params.camera),
            request.camera_override,
            error);
        if (!prepared.context.transient_camera) return false;
    }

    prepared.context.params.width = request.width;
    prepared.context.params.height = request.height;
    prepared.context.params.threads = request.threads;
    prepared.context.params.samples = request.samples;
    prepared.context.params.aa = request.aa;
    prepared.context.params.rdepth = request.rdepth;
    prepared.context.params.tile_size = request.tile_size;
    prepared.context.params.sample_distribution = request.sample_distribution;
    prepared.context.params.tile_order = request.tile_order;
    prepared.context.init();

    return true;
}

bool acquire_prepared_render(const render_request_t &request,
                             prepared_render_lease_t &lease,
                             std::string &error)
{
    lease.cacheable = can_reuse_cached_scene(request) && resolve_scene_cache_key(request, lease.cache_key);
    if (lease.cacheable) {
        prepared_scene_cache_t::handle().checkout(lease.cache_key, lease.prepared);
    }

    if (!lease.prepared) {
        lease.prepared.reset(new prepared_render_t());
        if (!load_scene_into_prepared(request, *lease.prepared, error)) {
            lease.prepared.reset();
            return false;
        }
    }

    if (!configure_prepared_render(request, *lease.prepared, error)) {
        lease.prepared.reset();
        return false;
    }
    return true;
}

void collect_photon_debug_points(xtcore::render::IIntegrator *integrator,
                                 render_result_t &result)
{
    xtcore::integrator::photon_mapping::Integrator *pm =
        dynamic_cast<xtcore::integrator::photon_mapping::Integrator *>(integrator);
    if (!pm) return;

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

render_result_t render_prepared_scene_to_png(const render_request_t &request,
                                             prepared_render_t &prepared,
                                             progress_callback_t on_progress,
                                             const std::atomic<bool> *abort_flag,
                                             bool encode_output)
{
    render_result_t result;
    xtcore::render::context_t &context = prepared.context;
    context.params.threads = request.threads;
    context.params.samples = request.samples;
    context.params.aa = request.aa;
    context.params.rdepth = request.rdepth;
    context.params.sample_distribution = request.sample_distribution;
    context.params.tile_order = request.tile_order;

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

    collect_photon_debug_points(integrator.get(), result);

    if (encode_output) {
        nimg::Pixmap framebuffer;
        xtcore::render::assemble(framebuffer, context);
        nimg::Pixmap ldr_framebuffer = framebuffer;
        xtcore::tonemapping::apply(ldr_framebuffer);
        if (!encode_png_memory(ldr_framebuffer, result.image_png, result.error)) {
            return result;
        }
        result.framebuffer = std::move(framebuffer);
    }

    result.tiles_done = result.tiles_total;
    result.ok = true;
    return result;
}

} // namespace

render_request_t::render_request_t()
    : scene_path()
    , scene_cache_key()
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
    , scene_cache_enabled(true)
    , save_to_gallery(true)
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
        } else if (type == "string") {
            // any value is valid
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
    openmp_thread_limit_guard_t thread_limit_guard(request.threads);
    if (request.render_mode == render_request_t::RENDER_MODE_PROGRESSIVE ||
        request.render_mode == render_request_t::RENDER_MODE_INCREMENTAL) {
        const size_t total_samples = (request.samples > 0) ? request.samples : 1;
        const size_t step = (request.render_mode == render_request_t::RENDER_MODE_INCREMENTAL)
            ? 1
            : progressive_pass_sample_step(total_samples);
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
        prepared_render_lease_t prepared;
        if (!acquire_prepared_render(request, prepared, result.error)) {
            return result;
        }

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
            render_result_t pass_result = render_prepared_scene_to_png(
                pass_request,
                *prepared.prepared,
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
                abort_flag,
                false);

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

            if (on_progress) {
                progress_tile_update_t pass_upd;
                pass_upd.has_rect = false;
                pass_upd.source_fb = &accum_fb;
                on_progress(PROGRESS_EVENT_PASS_FINISHED,
                            pass_index + 1,
                            pass_samples.size(),
                            nullptr, &pass_upd);
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

    prepared_render_lease_t prepared;
    if (!acquire_prepared_render(request, prepared, result.error)) {
        return result;
    }
    return render_prepared_scene_to_png(request, *prepared.prepared, on_progress, abort_flag, true);
}

} /* namespace common */
} /* namespace frontend */
} /* namespace xtracer */
