#include <cmath>
#include <cstdio>
#include <memory>
#include <limits>
#include <vector>

#include <nimg/luminance.h>
#include <nimg/pixmap.h>

#include <xtcore/context.h>
#include <xtcore/integrator.h>
#include <xtcore/scene.h>
#include <xtcore/strpool.h>
#include <xtcore/camera/perspective.h>
#include <xtcore/math/sphere.h>
#include <xtcore/material/dielectric.h>
#include <xtcore/material/lambert.h>
#include <xtcore/material/principled.h>
#include <xtcore/material/rough_dielectric.h>
#include <xtcore/material/sheen.h>
#include <xtcore/material/subsurface.h>
#include <xtcore/material/thin_dielectric.h>
#include <xtcore/material/thin_translucent.h>
#include <xtcore/sampler/sampler_col.h>
#include <xtcore/xtcore.h>
#include <xtcore/log.h>

namespace {

struct rgb_stats_t
{
    float r;
    float g;
    float b;
    float mean;
    float center_mean;
    float edge_mean;
    float edge_ratio;
    float radial_span;
    size_t samples;
};

bool collect_rgb_stats(nimg::Pixmap &frame,
                       size_t width,
                       size_t height,
                       const std::vector<unsigned char> &mask,
                       rgb_stats_t &out)
{
    if (mask.size() != width * height) return false;

    double sr = 0.0;
    double sg = 0.0;
    double sb = 0.0;
    double sx = 0.0;
    double sy = 0.0;
    size_t count = 0;

    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            if (!mask[y * width + x]) continue;
            const nimg::ColorRGBAf p = frame.pixel(x, y);
            if (!std::isfinite(p.r()) || !std::isfinite(p.g()) || !std::isfinite(p.b())) return false;
            sr += p.r();
            sg += p.g();
            sb += p.b();
            sx += static_cast<double>(x) + 0.5;
            sy += static_cast<double>(y) + 0.5;
            ++count;
        }
    }

    if (count == 0) return false;

    const double cx = sx / static_cast<double>(count);
    const double cy = sy / static_cast<double>(count);

    double rmax = 0.0;
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            if (!mask[y * width + x]) continue;
            const double dx = (static_cast<double>(x) + 0.5) - cx;
            const double dy = (static_cast<double>(y) + 0.5) - cy;
            rmax = std::max(rmax, std::sqrt(dx * dx + dy * dy));
        }
    }
    if (rmax <= 1e-6) return false;

    static const size_t k_bins = 4;
    double radial_sum[k_bins] = {0.0, 0.0, 0.0, 0.0};
    size_t radial_count[k_bins] = {0, 0, 0, 0};

    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            if (!mask[y * width + x]) continue;
            const double dx = (static_cast<double>(x) + 0.5) - cx;
            const double dy = (static_cast<double>(y) + 0.5) - cy;
            const double rn = std::min(0.999999, std::sqrt(dx * dx + dy * dy) / rmax);
            const size_t bin = std::min(k_bins - 1, static_cast<size_t>(rn * static_cast<double>(k_bins)));
            radial_sum[bin] += nimg::eval::luminance(frame.pixel(x, y));
            radial_count[bin] += 1;
        }
    }

    double radial_min = std::numeric_limits<double>::infinity();
    double radial_max = -std::numeric_limits<double>::infinity();
    for (size_t i = 0; i < k_bins; ++i) {
        if (radial_count[i] == 0) return false;
        const double bin_mean = radial_sum[i] / static_cast<double>(radial_count[i]);
        radial_min = std::min(radial_min, bin_mean);
        radial_max = std::max(radial_max, bin_mean);
    }

    out.r = static_cast<float>(sr / static_cast<double>(count));
    out.g = static_cast<float>(sg / static_cast<double>(count));
    out.b = static_cast<float>(sb / static_cast<double>(count));
    out.mean = (out.r + out.g + out.b) / 3.0f;
    out.center_mean = static_cast<float>(radial_sum[0] / static_cast<double>(radial_count[0]));
    out.edge_mean = static_cast<float>(radial_sum[k_bins - 1] / static_cast<double>(radial_count[k_bins - 1]));
    out.edge_ratio = static_cast<float>(out.edge_mean / std::max(1e-6f, out.center_mean));
    out.radial_span = static_cast<float>(radial_max - radial_min);
    out.samples = count;
    return true;
}

bool radial_uniformity_ok(const rgb_stats_t &stats,
                          float min_edge_ratio,
                          float max_radial_span)
{
    return stats.edge_ratio >= min_edge_ratio && stats.radial_span <= max_radial_span;
}

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
        integrator.reset(new xtcore::integrator::pathtracer_mis::Integrator());
    } else {
        integrator.reset(new xtcore::integrator::pathtracer::Integrator());
    }

    integrator->setup(ctx);
    integrator->render();

    nimg::Pixmap frame;
    xtcore::render::assemble(frame, ctx);
    return collect_rgb_stats(frame, ctx.params.width, ctx.params.height, mask, out);
}

bool run_dielectric_case(bool use_importance_sampling, rgb_stats_t &out)
{
    using xtcore::pool::str::add;

    xtcore::render::context_t ctx;
    ctx.params.width = 40;
    ctx.params.height = 40;
    ctx.params.tile_size = 16;
    ctx.params.threads = 1;
    ctx.params.samples = 256;
    ctx.params.aa = 1;
    ctx.params.rdepth = 8;
    ctx.params.tile_order = xtcore::render::TILE_ORDER_SCANLINE;

    xtcore::camera::Perspective *cam = new xtcore::camera::Perspective();
    cam->position = nmath::Vector3f(0.0f, 0.0f, 0.0f);
    cam->target = nmath::Vector3f(0.0f, 0.0f, 1.0f);
    cam->up = nmath::Vector3f(0.0f, 1.0f, 0.0f);
    cam->fov = 45.0f;
    const HASH_UINT64 cam_id = add("wf_d_cam");
    ctx.scene.m_cameras[cam_id] = cam;
    ctx.params.camera = cam_id;

    xtcore::surface::Sphere *sphere = new xtcore::surface::Sphere(nmath::Vector3f(0.0f, 0.0f, 3.0f), 1.0f);
    sphere->calc_aabb();
    const HASH_UINT64 geo_id = add("wf_d_sphere");
    ctx.scene.m_surface[geo_id] = sphere;

    xtcore::asset::material::Dielectric *mat = new xtcore::asset::material::Dielectric();
    mat->add_scalar("ior", 1.5f);
    mat->add_scalar("transparency", 1.0f);
    const HASH_UINT64 mat_id = add("wf_d_mat");
    ctx.scene.m_materials[mat_id] = mat;

    xtcore::asset::Object *obj = new xtcore::asset::Object();
    obj->surface = geo_id;
    obj->material = mat_id;
    const HASH_UINT64 obj_id = add("wf_d_obj");
    ctx.scene.m_objects[obj_id] = obj;

    xtcore::sampler::SolidColor *env = new xtcore::sampler::SolidColor();
    nimg::ColorRGBf white_env(1.0f, 1.0f, 1.0f);
    env->set(white_env);
    ctx.scene.m_environment = env;

    ctx.init();
    std::vector<unsigned char> mask = build_hit_mask(ctx);

    std::unique_ptr<xtcore::render::IIntegrator> integrator;
    if (use_importance_sampling) {
        integrator.reset(new xtcore::integrator::pathtracer_mis::Integrator());
    } else {
        integrator.reset(new xtcore::integrator::pathtracer::Integrator());
    }

    integrator->setup(ctx);
    integrator->render();

    nimg::Pixmap frame;
    xtcore::render::assemble(frame, ctx);
    return collect_rgb_stats(frame, ctx.params.width, ctx.params.height, mask, out);
}

bool run_rough_dielectric_case(bool use_importance_sampling, float roughness, rgb_stats_t &out)
{
    using xtcore::pool::str::add;

    xtcore::render::context_t ctx;
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
    const HASH_UINT64 cam_id = add("wf_rd_cam");
    ctx.scene.m_cameras[cam_id] = cam;
    ctx.params.camera = cam_id;

    xtcore::surface::Sphere *sphere = new xtcore::surface::Sphere(nmath::Vector3f(0.0f, 0.0f, 3.0f), 1.0f);
    sphere->calc_aabb();
    const HASH_UINT64 geo_id = add("wf_rd_sphere");
    ctx.scene.m_surface[geo_id] = sphere;

    xtcore::asset::material::RoughDielectric *mat = new xtcore::asset::material::RoughDielectric();
    xtcore::sampler::SolidColor *transmission = new xtcore::sampler::SolidColor();
    nimg::ColorRGBf white_trans(1.0f, 1.0f, 1.0f);
    transmission->set(white_trans);
    mat->add_sampler("transmission", transmission);
    mat->add_scalar("roughness", roughness);
    mat->add_scalar("ior", 1.5f);
    mat->add_scalar("transparency", 0.98f);
    const HASH_UINT64 mat_id = add("wf_rough_dielectric");
    ctx.scene.m_materials[mat_id] = mat;

    xtcore::asset::Object *obj = new xtcore::asset::Object();
    obj->surface = geo_id;
    obj->material = mat_id;
    const HASH_UINT64 obj_id = add("wf_rd_obj");
    ctx.scene.m_objects[obj_id] = obj;

    xtcore::sampler::SolidColor *env = new xtcore::sampler::SolidColor();
    nimg::ColorRGBf white_env(1.0f, 1.0f, 1.0f);
    env->set(white_env);
    ctx.scene.m_environment = env;

    ctx.init();
    std::vector<unsigned char> mask = build_hit_mask(ctx);

    std::unique_ptr<xtcore::render::IIntegrator> integrator;
    if (use_importance_sampling) {
        integrator.reset(new xtcore::integrator::pathtracer_mis::Integrator());
    } else {
        integrator.reset(new xtcore::integrator::pathtracer::Integrator());
    }

    integrator->setup(ctx);
    integrator->render();

    nimg::Pixmap frame;
    xtcore::render::assemble(frame, ctx);
    return collect_rgb_stats(frame, ctx.params.width, ctx.params.height, mask, out);
}

bool sample_absorbing_rough_dielectric_exit(nimg::ColorRGBf &out)
{
    xtcore::asset::material::RoughDielectric *mat = new xtcore::asset::material::RoughDielectric();
    xtcore::sampler::SolidColor *transmission = new xtcore::sampler::SolidColor();
    xtcore::sampler::SolidColor *absorption = new xtcore::sampler::SolidColor();
    nimg::ColorRGBf white_trans(1.0f, 1.0f, 1.0f);
    nimg::ColorRGBf blue_abs(0.25f, 0.55f, 0.92f);
    transmission->set(white_trans);
    absorption->set(blue_abs);
    mat->add_sampler("transmission", transmission);
    mat->add_sampler("absorption_color", absorption);
    mat->add_scalar("roughness", 0.02f);
    mat->add_scalar("ior", 1.5f);
    mat->add_scalar("transparency", 0.98f);
    mat->add_scalar("absorption_distance", 1.4f);

    xtcore::hit_record_t hit;
    hit.normal = nmath::Vector3f(0.0f, 0.0f, 1.0f);
    hit.point = nmath::Vector3f(0.0f, 0.0f, 0.0f);
    hit.texcoord = nmath::Vector3f(0.5f, 0.5f, 0.0f);
    hit.incident_direction = nmath::Vector3f(0.0f, 0.0f, 1.0f);
    hit.ior = 1.5f;

    bool found_transmission = false;
    for (int i = 0; i < 256; ++i) {
        xtcore::hit_result_t next;
        if (!mat->sample_path(next, hit)) continue;
        if (next.ior < 1.1f) {
            out = next.intensity;
            found_transmission = true;
            break;
        }
    }

    delete mat;
    return found_transmission;
}

bool verify_dielectric_reflection_direction()
{
    xtcore::asset::material::Dielectric mat;
    mat.add_scalar("ior", 1.5f);
    mat.add_scalar("transparency", 1.0f);

    xtcore::hit_record_t hit;
    hit.normal = nmath::Vector3f(0.0f, 0.0f, -1.0f);
    hit.point = nmath::Vector3f(0.0f, 0.0f, 0.0f);
    hit.texcoord = nmath::Vector3f(0.5f, 0.5f, 0.0f);
    hit.incident_direction = nmath::Vector3f(0.0f, 0.0f, 1.0f);
    hit.ior = 1.0f;

    for (int i = 0; i < 2048; ++i) {
        xtcore::hit_result_t next;
        if (!mat.sample_path(next, hit)) return false;
        if (next.ior > 1.1f) continue;
        return nmath::dot(next.ray.direction.normalized(), nmath::Vector3f(0.0f, 0.0f, -1.0f)) > 0.999f;
    }

    return false;
}

bool verify_thin_dielectric_reflection_direction()
{
    xtcore::asset::material::ThinDielectric mat;
    mat.add_scalar("ior", 1.45f);
    mat.add_scalar("roughness", 0.0f);
    mat.add_scalar("transparency", 1.0f);

    xtcore::hit_record_t hit;
    hit.normal = nmath::Vector3f(0.0f, 0.0f, -1.0f);
    hit.point = nmath::Vector3f(0.0f, 0.0f, 0.0f);
    hit.texcoord = nmath::Vector3f(0.5f, 0.5f, 0.0f);
    hit.incident_direction = nmath::Vector3f(0.0f, 0.0f, 1.0f);
    hit.ior = 1.0f;

    for (int i = 0; i < 2048; ++i) {
        xtcore::hit_result_t next;
        if (!mat.sample_path(next, hit)) return false;
        if (nmath::dot(next.ray.direction.normalized(), nmath::Vector3f(0.0f, 0.0f, 1.0f)) > 0.999f) continue;
        return nmath::dot(next.ray.direction.normalized(), nmath::Vector3f(0.0f, 0.0f, -1.0f)) > 0.999f;
    }

    return false;
}

bool run_principled_clearcoat_case(bool use_importance_sampling, float clearcoat, rgb_stats_t &out)
{
    using xtcore::pool::str::add;

    xtcore::render::context_t ctx;
    setup_scene(ctx, 1.0f);

    const HASH_UINT64 mat_id = add("wf_lambert");
    xtcore::asset::material::Principled *mat = new xtcore::asset::material::Principled();
    xtcore::sampler::SolidColor *base_color = new xtcore::sampler::SolidColor();
    nimg::ColorRGBf coat_base(0.7f, 0.15f, 0.12f);
    base_color->set(coat_base);
    mat->add_sampler("base_color", base_color);
    mat->add_scalar("metallic", 0.0f);
    mat->add_scalar("roughness", 0.32f);
    mat->add_scalar("ior", 1.5f);
    mat->add_scalar("clearcoat", clearcoat);
    mat->add_scalar("clearcoat_roughness", 0.06f);
    delete ctx.scene.m_materials[mat_id];
    ctx.scene.m_materials[mat_id] = mat;

    ctx.init();
    std::vector<unsigned char> mask = build_hit_mask(ctx);

    std::unique_ptr<xtcore::render::IIntegrator> integrator;
    if (use_importance_sampling) {
        integrator.reset(new xtcore::integrator::pathtracer_mis::Integrator());
    } else {
        integrator.reset(new xtcore::integrator::pathtracer::Integrator());
    }

    integrator->setup(ctx);
    integrator->render();

    nimg::Pixmap frame;
    xtcore::render::assemble(frame, ctx);
    return collect_rgb_stats(frame, ctx.params.width, ctx.params.height, mask, out);
}

bool run_principled_anisotropy_case(bool use_importance_sampling, float anisotropy, rgb_stats_t &out)
{
    using xtcore::pool::str::add;

    xtcore::render::context_t ctx;
    setup_scene(ctx, 1.0f);

    const HASH_UINT64 mat_id = add("wf_lambert");
    xtcore::asset::material::Principled *mat = new xtcore::asset::material::Principled();
    xtcore::sampler::SolidColor *base_color = new xtcore::sampler::SolidColor();
    nimg::ColorRGBf metal_base(0.88f, 0.82f, 0.74f);
    base_color->set(metal_base);
    mat->add_sampler("base_color", base_color);
    mat->add_scalar("metallic", 1.0f);
    mat->add_scalar("roughness", 0.24f);
    mat->add_scalar("anisotropy", anisotropy);
    mat->add_scalar("ior", 1.5f);
    delete ctx.scene.m_materials[mat_id];
    ctx.scene.m_materials[mat_id] = mat;

    ctx.init();
    std::vector<unsigned char> mask = build_hit_mask(ctx);

    std::unique_ptr<xtcore::render::IIntegrator> integrator;
    if (use_importance_sampling) {
        integrator.reset(new xtcore::integrator::pathtracer_mis::Integrator());
    } else {
        integrator.reset(new xtcore::integrator::pathtracer::Integrator());
    }

    integrator->setup(ctx);
    integrator->render();

    nimg::Pixmap frame;
    xtcore::render::assemble(frame, ctx);
    return collect_rgb_stats(frame, ctx.params.width, ctx.params.height, mask, out);
}

bool run_thin_dielectric_case(bool use_importance_sampling, float roughness, rgb_stats_t &out)
{
    using xtcore::pool::str::add;

    xtcore::render::context_t ctx;
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
    const HASH_UINT64 cam_id = add("wf_td_cam");
    ctx.scene.m_cameras[cam_id] = cam;
    ctx.params.camera = cam_id;

    xtcore::surface::Sphere *sphere = new xtcore::surface::Sphere(nmath::Vector3f(0.0f, 0.0f, 3.0f), 1.0f);
    sphere->calc_aabb();
    const HASH_UINT64 geo_id = add("wf_td_sphere");
    ctx.scene.m_surface[geo_id] = sphere;

    xtcore::asset::material::ThinDielectric *mat = new xtcore::asset::material::ThinDielectric();
    xtcore::sampler::SolidColor *transmission = new xtcore::sampler::SolidColor();
    nimg::ColorRGBf white_trans(1.0f, 1.0f, 1.0f);
    transmission->set(white_trans);
    mat->add_sampler("transmission", transmission);
    mat->add_scalar("roughness", roughness);
    mat->add_scalar("ior", 1.45f);
    mat->add_scalar("transparency", 0.98f);
    const HASH_UINT64 mat_id = add("wf_thin_dielectric");
    ctx.scene.m_materials[mat_id] = mat;

    xtcore::asset::Object *obj = new xtcore::asset::Object();
    obj->surface = geo_id;
    obj->material = mat_id;
    const HASH_UINT64 obj_id = add("wf_td_obj");
    ctx.scene.m_objects[obj_id] = obj;

    xtcore::sampler::SolidColor *env = new xtcore::sampler::SolidColor();
    nimg::ColorRGBf white_env(1.0f, 1.0f, 1.0f);
    env->set(white_env);
    ctx.scene.m_environment = env;

    ctx.init();
    std::vector<unsigned char> mask = build_hit_mask(ctx);

    std::unique_ptr<xtcore::render::IIntegrator> integrator;
    if (use_importance_sampling) {
        integrator.reset(new xtcore::integrator::pathtracer_mis::Integrator());
    } else {
        integrator.reset(new xtcore::integrator::pathtracer::Integrator());
    }

    integrator->setup(ctx);
    integrator->render();

    nimg::Pixmap frame;
    xtcore::render::assemble(frame, ctx);
    return collect_rgb_stats(frame, ctx.params.width, ctx.params.height, mask, out);
}

bool run_subsurface_case(bool use_importance_sampling,
                         float subsurface,
                         const nimg::ColorRGBf &radius,
                         float thickness,
                         rgb_stats_t &out)
{
    using xtcore::pool::str::add;

    xtcore::render::context_t ctx;
    setup_scene(ctx, 1.0f);

    const HASH_UINT64 mat_id = add("wf_lambert");
    xtcore::asset::material::Subsurface *mat = new xtcore::asset::material::Subsurface();
    xtcore::sampler::SolidColor *base_color = new xtcore::sampler::SolidColor();
    xtcore::sampler::SolidColor *scatter_color = new xtcore::sampler::SolidColor();
    xtcore::sampler::SolidColor *radius_color = new xtcore::sampler::SolidColor();
    nimg::ColorRGBf warm_base(1.0f, 1.0f, 1.0f);
    nimg::ColorRGBf warm_scatter(1.0f, 1.0f, 1.0f);
    nimg::ColorRGBf radius_copy(radius.r(), radius.g(), radius.b());
    base_color->set(warm_base);
    scatter_color->set(warm_scatter);
    radius_color->set(radius_copy);
    mat->add_sampler("base_color", base_color);
    mat->add_sampler("subsurface_color", scatter_color);
    mat->add_sampler("subsurface_radius", radius_color);
    mat->add_scalar("subsurface", subsurface);
    mat->add_scalar("thickness", thickness);
    delete ctx.scene.m_materials[mat_id];
    ctx.scene.m_materials[mat_id] = mat;

    ctx.init();
    std::vector<unsigned char> mask = build_hit_mask(ctx);

    std::unique_ptr<xtcore::render::IIntegrator> integrator;
    if (use_importance_sampling) {
        integrator.reset(new xtcore::integrator::pathtracer_mis::Integrator());
    } else {
        integrator.reset(new xtcore::integrator::pathtracer::Integrator());
    }

    integrator->setup(ctx);
    integrator->render();

    nimg::Pixmap frame;
    xtcore::render::assemble(frame, ctx);
    return collect_rgb_stats(frame, ctx.params.width, ctx.params.height, mask, out);
}

bool run_sheen_case(bool use_importance_sampling, float sheen, rgb_stats_t &out)
{
    using xtcore::pool::str::add;

    xtcore::render::context_t ctx;
    setup_scene(ctx, 1.0f);

    const HASH_UINT64 mat_id = add("wf_lambert");
    xtcore::asset::material::Sheen *mat = new xtcore::asset::material::Sheen();
    xtcore::sampler::SolidColor *base_color = new xtcore::sampler::SolidColor();
    xtcore::sampler::SolidColor *sheen_color = new xtcore::sampler::SolidColor();
    nimg::ColorRGBf base(0.40f, 0.28f, 0.20f);
    nimg::ColorRGBf sheen_tint(0.95f, 0.72f, 0.55f);
    base_color->set(base);
    sheen_color->set(sheen_tint);
    mat->add_sampler("base_color", base_color);
    mat->add_sampler("sheen_color", sheen_color);
    mat->add_scalar("sheen", sheen);
    delete ctx.scene.m_materials[mat_id];
    ctx.scene.m_materials[mat_id] = mat;

    ctx.init();
    std::vector<unsigned char> mask = build_hit_mask(ctx);

    std::unique_ptr<xtcore::render::IIntegrator> integrator;
    if (use_importance_sampling) {
        integrator.reset(new xtcore::integrator::pathtracer_mis::Integrator());
    } else {
        integrator.reset(new xtcore::integrator::pathtracer::Integrator());
    }

    integrator->setup(ctx);
    integrator->render();

    nimg::Pixmap frame;
    xtcore::render::assemble(frame, ctx);
    return collect_rgb_stats(frame, ctx.params.width, ctx.params.height, mask, out);
}

bool run_thin_translucent_case(bool use_importance_sampling,
                               float translucency,
                               float thickness,
                               const nimg::ColorRGBf &tint,
                               rgb_stats_t &out)
{
    using xtcore::pool::str::add;

    xtcore::render::context_t ctx;
    setup_scene(ctx, 1.0f);

    const HASH_UINT64 mat_id = add("wf_lambert");
    xtcore::asset::material::ThinTranslucent *mat = new xtcore::asset::material::ThinTranslucent();
    xtcore::sampler::SolidColor *base_color = new xtcore::sampler::SolidColor();
    xtcore::sampler::SolidColor *trans_color = new xtcore::sampler::SolidColor();
    nimg::ColorRGBf base(1.0f, 1.0f, 1.0f);
    nimg::ColorRGBf tint_copy(tint.r(), tint.g(), tint.b());
    base_color->set(base);
    trans_color->set(tint_copy);
    mat->add_sampler("base_color", base_color);
    mat->add_sampler("translucency_color", trans_color);
    mat->add_scalar("translucency", translucency);
    mat->add_scalar("thickness", thickness);
    delete ctx.scene.m_materials[mat_id];
    ctx.scene.m_materials[mat_id] = mat;

    ctx.init();
    std::vector<unsigned char> mask = build_hit_mask(ctx);

    std::unique_ptr<xtcore::render::IIntegrator> integrator;
    if (use_importance_sampling) {
        integrator.reset(new xtcore::integrator::pathtracer_mis::Integrator());
    } else {
        integrator.reset(new xtcore::integrator::pathtracer::Integrator());
    }

    integrator->setup(ctx);
    integrator->render();

    nimg::Pixmap frame;
    xtcore::render::assemble(frame, ctx);
    return collect_rgb_stats(frame, ctx.params.width, ctx.params.height, mask, out);
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

int verify_rough_dielectric(const char *name, bool use_importance_sampling)
{
    // Single-scatter GGX loses energy at grazing (G1 masking): clear ~0.83, frosted ~0.76.
    // Kulla-Conty multiple-scattering compensation is not implemented; thresholds reflect this.
    const float min_edge_ratio = 0.72f;
    const float max_radial_span = 0.28f;
    rgb_stats_t clear;
    rgb_stats_t frosted;
    if (!run_rough_dielectric_case(use_importance_sampling, 0.04f, clear)) {
        std::fprintf(stderr, "white_furnace_test: %s rough dielectric clear run failed\n", name);
        return 1;
    }
    if (!run_rough_dielectric_case(use_importance_sampling, 0.28f, frosted)) {
        std::fprintf(stderr, "white_furnace_test: %s rough dielectric frosted run failed\n", name);
        return 1;
    }

    if (clear.samples < 50 || frosted.samples < 50) {
        std::fprintf(stderr, "white_furnace_test: %s rough dielectric insufficient hit samples\n", name);
        return 1;
    }

    if (clear.mean < 0.15f || frosted.mean < 0.12f) {
        std::fprintf(stderr, "white_furnace_test: %s rough dielectric too dark (clear=%.4f frosted=%.4f)\n", name, clear.mean, frosted.mean);
        return 1;
    }

    if (std::fabs(clear.r - clear.g) > 0.1f || std::fabs(clear.r - clear.b) > 0.1f ||
        std::fabs(frosted.r - frosted.g) > 0.1f || std::fabs(frosted.r - frosted.b) > 0.1f) {
        std::fprintf(stderr, "white_furnace_test: %s rough dielectric channel imbalance too high\n", name);
        return 1;
    }

    if (!radial_uniformity_ok(clear, min_edge_ratio, max_radial_span) ||
        !radial_uniformity_ok(frosted, min_edge_ratio, max_radial_span)) {
        std::fprintf(stderr,
                     "white_furnace_test: %s rough dielectric radial bias too high "
                     "(clear ratio=%.4f span=%.4f, frosted ratio=%.4f span=%.4f)\n",
                     name, clear.edge_ratio, clear.radial_span, frosted.edge_ratio, frosted.radial_span);
        return 1;
    }

    std::printf("white_furnace_test: %s rough dielectric ok (clear=%.4f edge=%.4f center=%.4f ratio=%.4f span=%.4f  frosted=%.4f edge=%.4f center=%.4f ratio=%.4f span=%.4f)\n",
                name,
                clear.mean, clear.edge_mean, clear.center_mean, clear.edge_ratio, clear.radial_span,
                frosted.mean, frosted.edge_mean, frosted.center_mean, frosted.edge_ratio, frosted.radial_span);
    return 0;
}

int verify_absorbing_rough_dielectric(const char *name, bool use_importance_sampling)
{
    (void)use_importance_sampling;
    nimg::ColorRGBf absorbed;
    if (!sample_absorbing_rough_dielectric_exit(absorbed)) {
        std::fprintf(stderr, "white_furnace_test: %s absorbing rough dielectric sample failed\n", name);
        return 1;
    }
    if (!std::isfinite(absorbed.r()) || !std::isfinite(absorbed.g()) || !std::isfinite(absorbed.b())) {
        std::fprintf(stderr, "white_furnace_test: %s absorbing rough dielectric produced non-finite tint\n", name);
        return 1;
    }
    if (absorbed.r() <= 0.0f || absorbed.g() <= 0.0f || absorbed.b() <= 0.0f) {
        std::fprintf(stderr, "white_furnace_test: %s absorbing rough dielectric non-positive tint\n", name);
        return 1;
    }
    if (!(absorbed.b() > absorbed.g() && absorbed.g() > absorbed.r())) {
        std::fprintf(stderr, "white_furnace_test: %s absorbing rough dielectric tint ordering invalid (r=%.4f g=%.4f b=%.4f)\n",
                     name, absorbed.r(), absorbed.g(), absorbed.b());
        return 1;
    }
    std::printf("white_furnace_test: %s absorbing rough dielectric ok (r=%.4f g=%.4f b=%.4f)\n",
                name, absorbed.r(), absorbed.g(), absorbed.b());
    return 0;
}

int verify_dielectric(const char *name, bool use_importance_sampling)
{
    const float min_edge_ratio = 0.90f;
    const float max_radial_span = 0.12f;
    rgb_stats_t stats;
    if (!run_dielectric_case(use_importance_sampling, stats)) {
        std::fprintf(stderr, "white_furnace_test: %s dielectric run failed\n", name);
        return 1;
    }
    if (stats.samples < 50) {
        std::fprintf(stderr, "white_furnace_test: %s dielectric insufficient hit samples\n", name);
        return 1;
    }
    if (stats.mean < 0.75f || stats.mean > 1.1f) {
        std::fprintf(stderr, "white_furnace_test: %s dielectric out of range (mean=%.4f)\n", name, stats.mean);
        return 1;
    }
    if (!radial_uniformity_ok(stats, min_edge_ratio, max_radial_span)) {
        std::fprintf(stderr,
                     "white_furnace_test: %s dielectric radial bias too high "
                     "(edge=%.4f center=%.4f ratio=%.4f span=%.4f)\n",
                     name, stats.edge_mean, stats.center_mean, stats.edge_ratio, stats.radial_span);
        return 1;
    }
    std::printf("white_furnace_test: %s dielectric ok (mean=%.4f edge=%.4f center=%.4f ratio=%.4f span=%.4f)\n",
                name, stats.mean, stats.edge_mean, stats.center_mean, stats.edge_ratio, stats.radial_span);
    return 0;
}

int verify_delta_dielectric_reflection()
{
    if (!verify_dielectric_reflection_direction()) {
        std::fprintf(stderr, "white_furnace_test: dielectric delta reflection direction invalid\n");
        return 1;
    }
    if (!verify_thin_dielectric_reflection_direction()) {
        std::fprintf(stderr, "white_furnace_test: thin dielectric delta reflection direction invalid\n");
        return 1;
    }
    std::printf("white_furnace_test: delta dielectric reflection direction ok\n");
    return 0;
}

int verify_principled_clearcoat(const char *name, bool use_importance_sampling)
{
    rgb_stats_t base;
    rgb_stats_t coated;
    if (!run_principled_clearcoat_case(use_importance_sampling, 0.0f, base)) {
        std::fprintf(stderr, "white_furnace_test: %s principled base run failed\n", name);
        return 1;
    }
    if (!run_principled_clearcoat_case(use_importance_sampling, 1.0f, coated)) {
        std::fprintf(stderr, "white_furnace_test: %s principled clearcoat run failed\n", name);
        return 1;
    }

    if (base.samples < 50 || coated.samples < 50) {
        std::fprintf(stderr, "white_furnace_test: %s principled clearcoat insufficient hit samples\n", name);
        return 1;
    }

    if (base.mean < 0.05f || coated.mean < 0.05f || coated.mean > 1.1f) {
        std::fprintf(stderr, "white_furnace_test: %s principled clearcoat out of range (base=%.4f coated=%.4f)\n", name, base.mean, coated.mean);
        return 1;
    }

    if (coated.mean + 0.1f < base.mean) {
        std::fprintf(stderr, "white_furnace_test: %s principled clearcoat unexpectedly darkens (base=%.4f coated=%.4f)\n", name, base.mean, coated.mean);
        return 1;
    }

    std::printf("white_furnace_test: %s principled clearcoat ok (base=%.4f coated=%.4f)\n", name, base.mean, coated.mean);
    return 0;
}

int verify_principled_anisotropy(const char *name, bool use_importance_sampling)
{
    rgb_stats_t isotropic;
    rgb_stats_t anisotropic;
    if (!run_principled_anisotropy_case(use_importance_sampling, 0.0f, isotropic)) {
        std::fprintf(stderr, "white_furnace_test: %s principled isotropic metal run failed\n", name);
        return 1;
    }
    if (!run_principled_anisotropy_case(use_importance_sampling, 0.85f, anisotropic)) {
        std::fprintf(stderr, "white_furnace_test: %s principled anisotropic metal run failed\n", name);
        return 1;
    }
    if (isotropic.samples < 50 || anisotropic.samples < 50) {
        std::fprintf(stderr, "white_furnace_test: %s principled anisotropy insufficient hit samples\n", name);
        return 1;
    }
    if (isotropic.mean < 0.05f || anisotropic.mean < 0.05f || anisotropic.mean > 1.1f) {
        std::fprintf(stderr, "white_furnace_test: %s principled anisotropy out of range (iso=%.4f aniso=%.4f)\n", name, isotropic.mean, anisotropic.mean);
        return 1;
    }
    std::printf("white_furnace_test: %s principled anisotropy ok (iso=%.4f aniso=%.4f)\n", name, isotropic.mean, anisotropic.mean);
    return 0;
}

int verify_thin_dielectric(const char *name, bool use_importance_sampling)
{
    const float min_edge_ratio = 0.90f;
    const float max_radial_span = 0.12f;
    rgb_stats_t clear;
    rgb_stats_t frosted;
    if (!run_thin_dielectric_case(use_importance_sampling, 0.0f, clear)) {
        std::fprintf(stderr, "white_furnace_test: %s thin dielectric clear run failed\n", name);
        return 1;
    }
    if (!run_thin_dielectric_case(use_importance_sampling, 0.24f, frosted)) {
        std::fprintf(stderr, "white_furnace_test: %s thin dielectric frosted run failed\n", name);
        return 1;
    }
    if (clear.samples < 50 || frosted.samples < 50) {
        std::fprintf(stderr, "white_furnace_test: %s thin dielectric insufficient hit samples\n", name);
        return 1;
    }
    if (clear.mean < 0.2f || frosted.mean < 0.18f || clear.mean > 1.1f || frosted.mean > 1.1f) {
        std::fprintf(stderr, "white_furnace_test: %s thin dielectric out of range (clear=%.4f frosted=%.4f)\n", name, clear.mean, frosted.mean);
        return 1;
    }

    if (!radial_uniformity_ok(clear, min_edge_ratio, max_radial_span) ||
        !radial_uniformity_ok(frosted, min_edge_ratio, max_radial_span)) {
        std::fprintf(stderr,
                     "white_furnace_test: %s thin dielectric radial bias too high "
                     "(clear ratio=%.4f span=%.4f, frosted ratio=%.4f span=%.4f)\n",
                     name, clear.edge_ratio, clear.radial_span, frosted.edge_ratio, frosted.radial_span);
        return 1;
    }

    std::printf("white_furnace_test: %s thin dielectric ok (clear=%.4f edge=%.4f center=%.4f ratio=%.4f span=%.4f  frosted=%.4f edge=%.4f center=%.4f ratio=%.4f span=%.4f)\n",
                name,
                clear.mean, clear.edge_mean, clear.center_mean, clear.edge_ratio, clear.radial_span,
                frosted.mean, frosted.edge_mean, frosted.center_mean, frosted.edge_ratio, frosted.radial_span);
    return 0;
}

int verify_subsurface(const char *name, bool use_importance_sampling)
{
    rgb_stats_t mostly_diffuse;
    rgb_stats_t thin_translucent;
    rgb_stats_t thick_translucent;
    if (!run_subsurface_case(use_importance_sampling,
                             0.15f,
                             nimg::ColorRGBf(0.75f, 1.05f, 1.60f),
                             0.15f,
                             mostly_diffuse)) {
        std::fprintf(stderr, "white_furnace_test: %s subsurface low-scatter run failed\n", name);
        return 1;
    }
    if (!run_subsurface_case(use_importance_sampling,
                             0.75f,
                             nimg::ColorRGBf(0.75f, 1.05f, 1.60f),
                             0.20f,
                             thin_translucent)) {
        std::fprintf(stderr, "white_furnace_test: %s subsurface thin-scatter run failed\n", name);
        return 1;
    }
    if (!run_subsurface_case(use_importance_sampling,
                             0.75f,
                             nimg::ColorRGBf(0.75f, 1.05f, 1.60f),
                             0.90f,
                             thick_translucent)) {
        std::fprintf(stderr, "white_furnace_test: %s subsurface thick-scatter run failed\n", name);
        return 1;
    }
    if (mostly_diffuse.samples < 50 || thin_translucent.samples < 50 || thick_translucent.samples < 50) {
        std::fprintf(stderr, "white_furnace_test: %s subsurface insufficient hit samples\n", name);
        return 1;
    }
    if (mostly_diffuse.mean < 0.05f || thin_translucent.mean < 0.05f || thick_translucent.mean < 0.02f ||
        mostly_diffuse.mean > 1.1f || thin_translucent.mean > 1.1f || thick_translucent.mean > 1.1f) {
        std::fprintf(stderr, "white_furnace_test: %s subsurface out of range (low=%.4f thin=%.4f thick=%.4f)\n",
                     name, mostly_diffuse.mean, thin_translucent.mean, thick_translucent.mean);
        return 1;
    }
    if (!(thin_translucent.mean > thick_translucent.mean)) {
        std::fprintf(stderr, "white_furnace_test: %s subsurface thickness attenuation invalid (thin=%.4f thick=%.4f)\n",
                     name, thin_translucent.mean, thick_translucent.mean);
        return 1;
    }
    if (!(thin_translucent.b > thin_translucent.g && thin_translucent.g > thin_translucent.r)) {
        std::fprintf(stderr, "white_furnace_test: %s subsurface radius tint ordering invalid (r=%.4f g=%.4f b=%.4f)\n",
                     name, thin_translucent.r, thin_translucent.g, thin_translucent.b);
        return 1;
    }
    std::printf("white_furnace_test: %s subsurface ok (low=%.4f thin=%.4f thick=%.4f)\n",
                name, mostly_diffuse.mean, thin_translucent.mean, thick_translucent.mean);
    return 0;
}

int verify_sheen(const char *name, bool use_importance_sampling)
{
    rgb_stats_t low;
    rgb_stats_t high;
    if (!run_sheen_case(use_importance_sampling, 0.05f, low)) {
        std::fprintf(stderr, "white_furnace_test: %s sheen low run failed\n", name);
        return 1;
    }
    if (!run_sheen_case(use_importance_sampling, 0.85f, high)) {
        std::fprintf(stderr, "white_furnace_test: %s sheen high run failed\n", name);
        return 1;
    }
    if (low.samples < 50 || high.samples < 50) {
        std::fprintf(stderr, "white_furnace_test: %s sheen insufficient hit samples\n", name);
        return 1;
    }
    if (low.mean < 0.05f || high.mean < 0.05f || low.mean > 1.1f || high.mean > 1.1f) {
        std::fprintf(stderr, "white_furnace_test: %s sheen out of range (low=%.4f high=%.4f)\n", name, low.mean, high.mean);
        return 1;
    }
    if (high.mean + 0.15f < low.mean) {
        std::fprintf(stderr, "white_furnace_test: %s sheen unexpectedly darkens (low=%.4f high=%.4f)\n", name, low.mean, high.mean);
        return 1;
    }
    std::printf("white_furnace_test: %s sheen ok (low=%.4f high=%.4f)\n", name, low.mean, high.mean);
    return 0;
}

int verify_thin_translucent(const char *name, bool use_importance_sampling)
{
    rgb_stats_t low;
    rgb_stats_t thin;
    rgb_stats_t thick;
    if (!run_thin_translucent_case(use_importance_sampling, 0.18f, 0.10f, nimg::ColorRGBf(0.75f, 1.0f, 0.68f), low)) {
        std::fprintf(stderr, "white_furnace_test: %s thin translucent low run failed\n", name);
        return 1;
    }
    if (!run_thin_translucent_case(use_importance_sampling, 0.82f, 0.18f, nimg::ColorRGBf(0.70f, 1.0f, 0.62f), thin)) {
        std::fprintf(stderr, "white_furnace_test: %s thin translucent thin run failed\n", name);
        return 1;
    }
    if (!run_thin_translucent_case(use_importance_sampling, 0.82f, 0.70f, nimg::ColorRGBf(0.70f, 1.0f, 0.62f), thick)) {
        std::fprintf(stderr, "white_furnace_test: %s thin translucent thick run failed\n", name);
        return 1;
    }
    if (low.samples < 50 || thin.samples < 50 || thick.samples < 50) {
        std::fprintf(stderr, "white_furnace_test: %s thin translucent insufficient hit samples\n", name);
        return 1;
    }
    if (low.mean < 0.05f || thin.mean < 0.05f || thick.mean < 0.02f ||
        low.mean > 1.1f || thin.mean > 1.1f || thick.mean > 1.1f) {
        std::fprintf(stderr, "white_furnace_test: %s thin translucent out of range (low=%.4f thin=%.4f thick=%.4f)\n",
                     name, low.mean, thin.mean, thick.mean);
        return 1;
    }
    if (!(thin.mean > thick.mean)) {
        std::fprintf(stderr, "white_furnace_test: %s thin translucent thickness attenuation invalid (thin=%.4f thick=%.4f)\n",
                     name, thin.mean, thick.mean);
        return 1;
    }
    if (!(thin.g > thin.r && thin.g > thin.b)) {
        std::fprintf(stderr, "white_furnace_test: %s thin translucent green tint invalid (r=%.4f g=%.4f b=%.4f)\n",
                     name, thin.r, thin.g, thin.b);
        return 1;
    }
    std::printf("white_furnace_test: %s thin translucent ok (low=%.4f thin=%.4f thick=%.4f)\n",
                name, low.mean, thin.mean, thick.mean);
    return 0;
}

} // namespace

int main()
{
    xtcore::init();
    xtcore::Log::handle().echo(false);

    int rc = 0;
    if (verify_delta_dielectric_reflection() != 0) rc = 1;
    if (verify_dielectric("pathtracer", false) != 0) rc = 1;
    if (verify_dielectric("pathtracer_mis", true) != 0) rc = 1;
    if (verify_renderer("pathtracer", false) != 0) rc = 1;
    if (verify_renderer("pathtracer_mis", true) != 0) rc = 1;
    if (verify_principled_clearcoat("pathtracer", false) != 0) rc = 1;
    if (verify_principled_clearcoat("pathtracer_mis", true) != 0) rc = 1;
    if (verify_principled_anisotropy("pathtracer", false) != 0) rc = 1;
    if (verify_principled_anisotropy("pathtracer_mis", true) != 0) rc = 1;
    if (verify_sheen("pathtracer", false) != 0) rc = 1;
    if (verify_sheen("pathtracer_mis", true) != 0) rc = 1;
    if (verify_thin_translucent("pathtracer", false) != 0) rc = 1;
    if (verify_thin_translucent("pathtracer_mis", true) != 0) rc = 1;
    if (verify_thin_dielectric("pathtracer", false) != 0) rc = 1;
    if (verify_thin_dielectric("pathtracer_mis", true) != 0) rc = 1;
    if (verify_subsurface("pathtracer", false) != 0) rc = 1;
    if (verify_subsurface("pathtracer_mis", true) != 0) rc = 1;
    if (verify_rough_dielectric("pathtracer", false) != 0) rc = 1;
    if (verify_rough_dielectric("pathtracer_mis", true) != 0) rc = 1;
    if (verify_absorbing_rough_dielectric("pathtracer", false) != 0) rc = 1;
    if (verify_absorbing_rough_dielectric("pathtracer_mis", true) != 0) rc = 1;
    xtcore::deinit();
    if (rc != 0) return rc;

    std::printf("white_furnace_test: ok\n");
    return 0;
}
