#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include <nimg/img.h>
#include <xtcore/integrator.h>
#include <xtcore/parseutil.h>
#include <xtcore/strpool.h>
#include <xtcore/tonemapping/tonemapping.h>
#include <xtcore/xtcore.h>

namespace {

std::string g_last_error;
bool g_xtcore_initialized = false;

struct wasm_render_session_t
{
    bool active;
    bool done;
    bool failed;
    bool auxiliary_ready;
    std::string error;
    xtcore::render::context_t context;
    std::unique_ptr<xtcore::render::IIntegrator> integrator;
    size_t tiles_done;
    size_t tiles_total;

    wasm_render_session_t()
        : active(false)
        , done(false)
        , failed(false)
        , auxiliary_ready(false)
        , error()
        , context()
        , integrator()
        , tiles_done(0)
        , tiles_total(0)
    {}
};

wasm_render_session_t g_session;

bool ensure_xtcore_initialized()
{
    if (g_xtcore_initialized) return true;
    if (xtcore::init() != 0) return false;
    g_xtcore_initialized = true;
    return true;
}

const char *safe_cstr(const char *s)
{
    return s ? s : "";
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

bool read_file_bytes(const char *path, std::vector<unsigned char> &out)
{
    std::ifstream in(path, std::ios::binary);
    if (!in.good()) return false;
    out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return true;
}

typedef int (*save_fn_t)(const char *, nimg::Pixmap &);

bool encode_image_memory(nimg::Pixmap &pixmap,
                         const char *format_ext,
                         save_fn_t save_fn,
                         std::vector<unsigned char> &out,
                         std::string &error)
{
    if (!format_ext || !(*format_ext) || !save_fn) {
        error = "invalid image encoder";
        return false;
    }

    const std::string tmp_path = std::string("/tmp/xtracer_wasm.") + format_ext;
    mkdir("/tmp", 0777);

    nimg::Pixmap ldr;
    nimg::Pixmap *to_encode = &pixmap;
    if (std::strcmp(format_ext, "png") == 0) {
        ldr = pixmap;
        xtcore::tonemapping::apply(ldr);
        to_encode = &ldr;
    }

    int save_err = save_fn(tmp_path.c_str(), *to_encode);
    if (save_err != 0) {
        error = std::string("failed to encode ") + format_ext;
        return false;
    }

    bool ok = read_file_bytes(tmp_path.c_str(), out);
    unlink(tmp_path.c_str());
    if (!ok) {
        error = std::string("failed to read generated ") + format_ext;
        return false;
    }
    return true;
}

unsigned char *snapshot_image_encoded(int final_only,
                                      int *out_size,
                                      const char *format_ext,
                                      save_fn_t save_fn)
{
    if (!out_size) {
        g_last_error = "out_size pointer is required";
        return nullptr;
    }

    *out_size = 0;
    g_last_error.clear();

    if (g_session.failed) {
        g_last_error = g_session.error.empty() ? "render failed" : g_session.error;
        return nullptr;
    }

    if (g_session.tiles_total == 0) {
        g_last_error = "no active render session";
        return nullptr;
    }

    if (final_only && !g_session.done) {
        g_last_error = "final image not ready";
        return nullptr;
    }

    nimg::Pixmap framebuffer;
    xtcore::render::assemble(framebuffer, g_session.context);

    std::vector<unsigned char> image;
    std::string error;
    if (!encode_image_memory(framebuffer, format_ext, save_fn, image, error)) {
        g_last_error = error.empty() ? "failed to encode image" : error;
        return nullptr;
    }

    if (image.empty()) {
        g_last_error = "render produced no image";
        return nullptr;
    }

    unsigned char *out = static_cast<unsigned char *>(std::malloc(image.size()));
    if (!out) {
        g_last_error = "out of memory";
        return nullptr;
    }

    std::memcpy(out, image.data(), image.size());
    *out_size = static_cast<int>(image.size());
    return out;
}

void clear_session()
{
    if (g_session.integrator && g_session.auxiliary_ready) {
        g_session.integrator->clean_auxiliary();
    }
    g_session = wasm_render_session_t();
}

bool prepare_session(const char *scene_path,
                     const char *integrator,
                     const char *camera,
                     const char *variant,
                     int width,
                     int height,
                     int samples,
                     int aa,
                     int tile_size,
                     int threads,
                     int rdepth)
{
    clear_session();

    if (!ensure_xtcore_initialized()) {
        g_last_error = "xtcore init failed";
        g_session.failed = true;
        g_session.error = g_last_error;
        return false;
    }

    if (!scene_path || !(*scene_path)) {
        g_last_error = "scene path is empty";
        g_session.failed = true;
        g_session.error = g_last_error;
        return false;
    }

    if (width <= 0 || height <= 0) {
        g_last_error = "invalid frame size";
        g_session.failed = true;
        g_session.error = g_last_error;
        return false;
    }

    const char *variant_name = (variant && *variant) ? variant : nullptr;
    int load_err = xtcore::io::scn::load(&(g_session.context.scene), scene_path, nullptr, variant_name);
    if (load_err) {
        g_last_error = "failed to load scene";
        g_session.failed = true;
        g_session.error = g_last_error;
        return false;
    }

    if (camera && *camera) {
        g_session.context.params.camera = xtcore::pool::str::add(camera);
    } else {
        auto first_cam = g_session.context.scene.m_cameras.begin();
        if (first_cam != g_session.context.scene.m_cameras.end()) {
            g_session.context.params.camera = (*first_cam).first;
        } else {
            g_session.context.params.camera = HASH_ID_INVALID;
        }
    }

    if (g_session.context.params.camera == HASH_ID_INVALID
        || !g_session.context.scene.get_camera(g_session.context.params.camera)) {
        g_last_error = "no valid camera found";
        g_session.failed = true;
        g_session.error = g_last_error;
        return false;
    }

    g_session.context.params.width = static_cast<size_t>(width);
    g_session.context.params.height = static_cast<size_t>(height);
    g_session.context.params.threads = static_cast<size_t>(threads >= 0 ? threads : 0);
    g_session.context.params.samples = static_cast<size_t>(samples > 0 ? samples : 1);
    g_session.context.params.aa = static_cast<size_t>(aa > 0 ? aa : 1);
    g_session.context.params.rdepth = static_cast<size_t>(rdepth > 0 ? rdepth : 3);
    g_session.context.params.tile_size = static_cast<size_t>(tile_size > 0 ? tile_size : 32);
    g_session.context.init();

    g_session.integrator = create_integrator(safe_cstr(integrator));
    if (!g_session.integrator) {
        g_last_error = "integrator not supported";
        g_session.failed = true;
        g_session.error = g_last_error;
        return false;
    }

    g_session.integrator->setup(g_session.context);
    xtcore::render::order(g_session.context.tiles, g_session.context.params.tile_order);
    g_session.integrator->setup_auxiliary();
    g_session.auxiliary_ready = true;

    g_session.tiles_done = 0;
    g_session.tiles_total = g_session.context.tiles.size();
    g_session.active = (g_session.tiles_total > 0);
    g_session.done = (g_session.tiles_total == 0);
    g_session.failed = false;
    g_session.error.clear();

    if (g_session.done && g_session.integrator && g_session.auxiliary_ready) {
        g_session.integrator->clean_auxiliary();
        g_session.auxiliary_ready = false;
    }

    return true;
}

void fail_session(const std::string &error)
{
    g_session.error = error;
    g_last_error = error;
    g_session.failed = true;
    g_session.active = false;
    if (g_session.integrator && g_session.auxiliary_ready) {
        g_session.integrator->clean_auxiliary();
        g_session.auxiliary_ready = false;
    }
}

void finish_session()
{
    g_session.active = false;
    g_session.done = true;
    if (g_session.integrator && g_session.auxiliary_ready) {
        g_session.integrator->clean_auxiliary();
        g_session.auxiliary_ready = false;
    }
}

} // namespace

extern "C" {

const char *xtracer_wasm_get_last_error()
{
    return g_last_error.c_str();
}

void xtracer_wasm_free(void *ptr)
{
    std::free(ptr);
}

int xtracer_wasm_render_begin(const char *scene_path,
                              const char *integrator,
                              const char *camera,
                              const char *variant,
                              int width,
                              int height,
                              int samples,
                              int aa,
                              int tile_size,
                              int threads,
                              int rdepth)
{
    g_last_error.clear();
    return prepare_session(scene_path, integrator, camera, variant, width, height, samples, aa, tile_size, threads, rdepth) ? 1 : 0;
}

int xtracer_wasm_render_step(int max_tiles)
{
    if (g_session.failed) return -1;
    if (!g_session.active) return 0;

    size_t budget = static_cast<size_t>(max_tiles > 0 ? max_tiles : 1);
    size_t processed = 0;
    xtcore::render::params_t *p = &(g_session.context.params);

    while (budget > 0 && g_session.tiles_done < g_session.tiles_total) {
        xtcore::render::tile_t *tile = &(g_session.context.tiles[g_session.tiles_done]);
        tile->init();
        xtcore::antialiasing::produce(tile, p->sample_distribution, p->aa, p->samples);

        try {
            g_session.integrator->render_tile(tile);
        } catch (...) {
            fail_session("render tile failed");
            return -1;
        }

        tile->submit();
        ++g_session.tiles_done;
        ++processed;
        --budget;
    }

    if (g_session.tiles_done >= g_session.tiles_total) {
        finish_session();
    }

    return static_cast<int>(processed);
}

int xtracer_wasm_render_tiles_done()
{
    return static_cast<int>(g_session.tiles_done);
}

int xtracer_wasm_render_tiles_total()
{
    return static_cast<int>(g_session.tiles_total);
}

int xtracer_wasm_render_is_done()
{
    return g_session.done ? 1 : 0;
}

unsigned char *xtracer_wasm_render_snapshot_png(int final_only, int *out_size)
{
    return snapshot_image_encoded(final_only, out_size, "png", nimg::io::save::png);
}

unsigned char *xtracer_wasm_render_snapshot_exr(int final_only, int *out_size)
{
    return snapshot_image_encoded(final_only, out_size, "exr", nimg::io::save::exr);
}

unsigned char *xtracer_wasm_render_snapshot_hdr(int final_only, int *out_size)
{
    return snapshot_image_encoded(final_only, out_size, "hdr", nimg::io::save::hdr);
}

unsigned char *xtracer_wasm_render_png(const char *scene_path,
                                       const char *integrator,
                                       const char *camera,
                                       const char *variant,
                                       int width,
                                       int height,
                                       int samples,
                                       int aa,
                                       int tile_size,
                                       int threads,
                                       int rdepth,
                                       int *out_size)
{
    if (!out_size) {
        g_last_error = "out_size pointer is required";
        return nullptr;
    }

    *out_size = 0;

    if (!xtracer_wasm_render_begin(scene_path, integrator, camera, variant, width, height, samples, aa, tile_size, threads, rdepth)) {
        return nullptr;
    }

    while (!xtracer_wasm_render_is_done()) {
        if (xtracer_wasm_render_step(1) < 0) {
            return nullptr;
        }
    }

    return xtracer_wasm_render_snapshot_png(1, out_size);
}

} // extern "C"
