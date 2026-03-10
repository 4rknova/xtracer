#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

#include <nimg/pixmap.h>

#include <xtcore/context.h>
#include <xtcore/integrator.h>
#include <xtcore/scene.h>
#include <xtcore/strpool.h>
#include <xtcore/camera/perspective.h>
#include <xtcore/math/sphere.h>
#include <xtcore/material/lambert.h>
#include <xtcore/sampler_col.h>
#include <xtcore/xtcore.h>
#include <xtcore/log.h>

namespace {

struct rgb_stats_t
{
    float r;
    float g;
    float b;
    float mean;
    size_t samples;
};

void setup_scene(xtcore::render::context_t &ctx, float albedo)
{
    using xtcore::pool::str::add;

    ctx.params.width = 40;
    ctx.params.height = 40;
    ctx.params.tile_size = 16;
    ctx.params.threads = 1;
    ctx.params.samples = 128;
    ctx.params.aa = 1;
    ctx.params.rdepth = 8;
    ctx.params.tile_order = xtcore::render::TILE_ORDER_SCANLINE;

    xtcore::camera::Perspective *cam = new xtcore::camera::Perspective();
    cam->position = nmath::Vector3f(0.0f, 0.0f, 0.0f);
    cam->target = nmath::Vector3f(0.0f, 0.0f, 1.0f);
    cam->up = nmath::Vector3f(0.0f, 1.0f, 0.0f);
    cam->fov = 45.0f;

    const HASH_UINT64 cam_id = add("wf_cam");
    ctx.scene.m_cameras[cam_id] = cam;
    ctx.params.camera = cam_id;

    xtcore::surface::Sphere *sphere = new xtcore::surface::Sphere(nmath::Vector3f(0.0f, 0.0f, 3.0f), 1.0f);
    sphere->calc_aabb();
    const HASH_UINT64 geo_id = add("wf_sphere");
    ctx.scene.m_surface[geo_id] = sphere;

    xtcore::asset::material::Lambert *mat = new xtcore::asset::material::Lambert();
    xtcore::sampler::SolidColor *diffuse = new xtcore::sampler::SolidColor();
    nimg::ColorRGBf kd(albedo, albedo, albedo);
    diffuse->set(kd);
    mat->add_sampler("diffuse", diffuse);
    const HASH_UINT64 mat_id = add("wf_lambert");
    ctx.scene.m_materials[mat_id] = mat;

    xtcore::asset::Object *obj = new xtcore::asset::Object();
    obj->surface = geo_id;
    obj->material = mat_id;
    const HASH_UINT64 obj_id = add("wf_obj");
    ctx.scene.m_objects[obj_id] = obj;

    xtcore::sampler::SolidColor *env = new xtcore::sampler::SolidColor();
    nimg::ColorRGBf white(1.0f, 1.0f, 1.0f);
    env->set(white);
    ctx.scene.m_environment = env;

    ctx.init();
}

std::vector<unsigned char> build_hit_mask(xtcore::render::context_t &ctx)
{
    std::vector<unsigned char> mask(ctx.params.width * ctx.params.height, 0);
    xtcore::asset::ICamera *cam = ctx.scene.get_camera(ctx.params.camera);
    if (!cam) return mask;

    for (size_t y = 0; y < ctx.params.height; ++y) {
        for (size_t x = 0; x < ctx.params.width; ++x) {
            xtcore::Ray ray = cam->get_primary_ray(
                static_cast<float>(x) + 0.5f,
                static_cast<float>(y) + 0.5f,
                static_cast<float>(ctx.params.width),
                static_cast<float>(ctx.params.height));

            xtcore::hit_record_t hr;
            if (ctx.scene.intersection(ray, hr)) {
                mask[y * ctx.params.width + x] = 1;
            }
        }
    }

    return mask;
}

bool run_case(bool use_importance_sampling, float albedo, rgb_stats_t &out)
{
    xtcore::render::context_t ctx;
    setup_scene(ctx, albedo);

    std::vector<unsigned char> mask = build_hit_mask(ctx);

    std::unique_ptr<xtcore::render::IIntegrator> integrator;
    if (use_importance_sampling) {
        integrator.reset(new xtcore::integrator::pathtracer_is::Integrator());
    } else {
        integrator.reset(new xtcore::integrator::pathtracer::Integrator());
    }

    integrator->setup(ctx);
    integrator->render();

    nimg::Pixmap frame;
    xtcore::render::assemble(frame, ctx);

    double sr = 0.0;
    double sg = 0.0;
    double sb = 0.0;
    size_t count = 0;

    for (size_t y = 0; y < ctx.params.height; ++y) {
        for (size_t x = 0; x < ctx.params.width; ++x) {
            if (!mask[y * ctx.params.width + x]) continue;
            const nimg::ColorRGBAf p = frame.pixel(x, y);
            if (!std::isfinite(p.r()) || !std::isfinite(p.g()) || !std::isfinite(p.b())) return false;
            sr += p.r();
            sg += p.g();
            sb += p.b();
            ++count;
        }
    }

    if (count == 0) return false;

    out.r = static_cast<float>(sr / static_cast<double>(count));
    out.g = static_cast<float>(sg / static_cast<double>(count));
    out.b = static_cast<float>(sb / static_cast<double>(count));
    out.mean = (out.r + out.g + out.b) / 3.0f;
    out.samples = count;

    return true;
}

int verify_renderer(const char *name, bool use_importance_sampling)
{
    rgb_stats_t white;
    rgb_stats_t gray;

    if (!run_case(use_importance_sampling, 1.0f, white)) {
        std::fprintf(stderr, "white_furnace_test: %s white run failed\n", name);
        return 1;
    }
    if (!run_case(use_importance_sampling, 0.5f, gray)) {
        std::fprintf(stderr, "white_furnace_test: %s gray run failed\n", name);
        return 1;
    }

    if (white.samples < 50 || gray.samples < 50) {
        std::fprintf(stderr, "white_furnace_test: %s insufficient hit samples\n", name);
        return 1;
    }

    if (!(white.mean > gray.mean)) {
        std::fprintf(stderr, "white_furnace_test: %s expected white > gray, got %.4f <= %.4f\n", name, white.mean, gray.mean);
        return 1;
    }

    const float ratio = gray.mean / (white.mean > 1e-6f ? white.mean : 1e-6f);
    if (ratio < 0.35f || ratio > 0.65f) {
        std::fprintf(stderr, "white_furnace_test: %s unexpected gray/white ratio %.4f\n", name, ratio);
        return 1;
    }

    const float white_spread = std::fmax(std::fmax(std::fabs(white.r - white.g), std::fabs(white.r - white.b)), std::fabs(white.g - white.b));
    const float gray_spread = std::fmax(std::fmax(std::fabs(gray.r - gray.g), std::fabs(gray.r - gray.b)), std::fabs(gray.g - gray.b));

    if (white_spread > 0.08f || gray_spread > 0.08f) {
        std::fprintf(stderr, "white_furnace_test: %s channel imbalance too high (white=%.4f gray=%.4f)\n", name, white_spread, gray_spread);
        return 1;
    }

    if (white.mean < 0.05f || white.mean > 1.1f || gray.mean < 0.02f || gray.mean > 0.9f) {
        std::fprintf(stderr, "white_furnace_test: %s out-of-range means (white=%.4f gray=%.4f)\n", name, white.mean, gray.mean);
        return 1;
    }

    std::printf("white_furnace_test: %s ok (white=%.4f gray=%.4f ratio=%.4f)\n", name, white.mean, gray.mean, ratio);
    return 0;
}

} // namespace

int main()
{
    xtcore::init();
    xtcore::Log::handle().echo(false);

    int rc = 0;
    if (verify_renderer("pathtracer", false) != 0) rc = 1;
    if (verify_renderer("pathtracer_is", true) != 0) rc = 1;
    xtcore::deinit();
    if (rc != 0) return rc;

    std::printf("white_furnace_test: ok\n");
    return 0;
}
