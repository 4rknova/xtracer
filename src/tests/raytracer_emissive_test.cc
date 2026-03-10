#include <cmath>
#include <cstdio>

#include <nimg/pixmap.h>

#include <xtcore/context.h>
#include <xtcore/integrator.h>
#include <xtcore/scene.h>
#include <xtcore/strpool.h>
#include <xtcore/xtcore.h>
#include <xtcore/log.h>
#include <xtcore/camera/perspective.h>
#include <xtcore/math/triangle.h>
#include <xtcore/material/emissive.h>
#include <xtcore/sampler_col.h>

namespace {

int fail(const char *msg)
{
    std::fprintf(stderr, "raytracer_emissive_test: %s\n", msg);
    return 1;
}

bool finite_rgb(const nimg::ColorRGBAf &c)
{
    return std::isfinite(c.r()) && std::isfinite(c.g()) && std::isfinite(c.b());
}

} // namespace

int main()
{
    xtcore::init();
    xtcore::Log::handle().echo(false);

    xtcore::render::context_t ctx;
    ctx.params.width = 32;
    ctx.params.height = 32;
    ctx.params.tile_size = 16;
    ctx.params.threads = 1;
    ctx.params.samples = 1;
    ctx.params.aa = 1;
    ctx.params.rdepth = 2;
    ctx.params.tile_order = xtcore::render::TILE_ORDER_SCANLINE;

    using xtcore::pool::str::add;

    xtcore::camera::Perspective *cam = new xtcore::camera::Perspective();
    cam->position = nmath::Vector3f(0.0f, 0.0f, 0.0f);
    cam->target = nmath::Vector3f(0.0f, 0.0f, 1.0f);
    cam->up = nmath::Vector3f(0.0f, 1.0f, 0.0f);
    cam->fov = 45.0f;
    const HASH_UINT64 cam_id = add("rt_emissive_cam");
    ctx.scene.m_cameras[cam_id] = cam;
    ctx.params.camera = cam_id;

    xtcore::surface::Triangle *tri = new xtcore::surface::Triangle();
    tri->v[0] = nmath::Vector3f(-1.2f, -1.0f, 3.0f);
    tri->v[1] = nmath::Vector3f(1.2f, -1.0f, 3.0f);
    tri->v[2] = nmath::Vector3f(0.0f, 1.2f, 3.0f);
    tri->calc_aabb();
    const HASH_UINT64 geo_id = add("rt_emissive_tri");
    ctx.scene.m_surface[geo_id] = tri;

    xtcore::asset::material::Emissive *mat = new xtcore::asset::material::Emissive();
    xtcore::sampler::SolidColor *emissive = new xtcore::sampler::SolidColor();
    nimg::ColorRGBf white(1.0f, 1.0f, 1.0f);
    emissive->set(white);
    mat->add_sampler("emissive", emissive);
    const HASH_UINT64 mat_id = add("rt_emissive_mat");
    ctx.scene.m_materials[mat_id] = mat;

    xtcore::asset::Object *obj = new xtcore::asset::Object();
    obj->surface = geo_id;
    obj->material = mat_id;
    const HASH_UINT64 obj_id = add("rt_emissive_obj");
    ctx.scene.m_objects[obj_id] = obj;

    xtcore::sampler::SolidColor *env = new xtcore::sampler::SolidColor();
    nimg::ColorRGBf black(0.0f, 0.0f, 0.0f);
    env->set(black);
    ctx.scene.m_environment = env;

    ctx.init();

    xtcore::integrator::raytracer::Integrator integrator;
    integrator.setup(ctx);
    integrator.render();

    nimg::Pixmap frame;
    xtcore::render::assemble(frame, ctx);

    const nimg::ColorRGBAf center = frame.pixel(ctx.params.width / 2, ctx.params.height / 2);
    const nimg::ColorRGBAf corner = frame.pixel(0, 0);

    xtcore::deinit();

    if (!finite_rgb(center) || !finite_rgb(corner)) return fail("non-finite pixel values");

    const float center_mean = (center.r() + center.g() + center.b()) / 3.0f;
    const float corner_mean = (corner.r() + corner.g() + corner.b()) / 3.0f;

    if (center_mean < 0.2f) return fail("center pixel is not emissive/bright");
    if (corner_mean > 0.05f) return fail("background corner is unexpectedly bright");

    std::printf("raytracer_emissive_test: ok (center=%.4f corner=%.4f)\n", center_mean, corner_mean);
    return 0;
}
