#include "render_service.h"

#include <atomic>
#include <chrono>
#include <fstream>
#include <iterator>
#include <memory>
#include <unistd.h>
#include <xtcore/strpool.h>
#include <xtcore/parseutil.h>
#include <xtcore/integrator.h>
#include <nimg/img.h>

namespace xtracer {
namespace frontend {
namespace common {

namespace {

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

static const integrator_info_t k_integrators[] = {
      { "pathtracer", "Pathtracer (Brute Force)" }
    , { "pathtracer_is", "Pathtracer (IS)" }
    , { "photon_mapping", "Photon Mapping" }
    , { "depth"     , "Depth" }
    , { "stencil"   , "Stencil" }
    , { "normal"    , "Normal" }
    , { "uv"        , "UV" }
    , { "emission"  , "Emission" }
    , { "ao"        , "Ambient Occlusion" }
};

std::unique_ptr<xtcore::render::IIntegrator> create_integrator(const std::string &name)
{
    if      (name == "pathtracer") return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::pathtracer::Integrator());
    else if (name == "pathtracer_is") return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::pathtracer_is::Integrator());
    else if (name == "photon_mapping") return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::photon_mapping::Integrator());
    else if (name == "depth")      return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::depth::Integrator());
    else if (name == "stencil")    return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::stencil::Integrator());
    else if (name == "normal")     return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::normal::Integrator());
    else if (name == "uv")         return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::uv::Integrator());
    else if (name == "emission")   return std::unique_ptr<xtcore::render::IIntegrator>(new xtcore::integrator::emission::Integrator());
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
}

} // namespace

render_request_t::render_request_t()
    : scene_path()
    , integrator("pathtracer")
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
