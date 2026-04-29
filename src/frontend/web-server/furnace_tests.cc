#include "furnace_tests.h"

#include <cmath>
#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <vector>

#include <nimg/img.h>
#include <nimg/luminance.h>
#include <nimg/pixmap.h>

#include <xtcore/context.h>
#include <xtcore/integrator.h>
#include <xtcore/scene.h>
#include <xtcore/strpool.h>
#include <xtcore/camera/perspective.h>
#include <xtcore/math/sphere.h>
#include <xtcore/material/lambert.h>
#include <xtcore/material/principled.h>
#include <xtcore/material/rough_dielectric.h>
#include <xtcore/material/sheen.h>
#include <xtcore/material/subsurface.h>
#include <xtcore/material/thin_dielectric.h>
#include <xtcore/material/thin_translucent.h>
#include <xtcore/sampler/sampler_col.h>
#include <xtcore/tonemapping/tonemapping.h>
#include <xtcore/xtcore.h>
#include <xtcore/log.h>

namespace xtracer {
namespace frontend {
namespace web {

namespace {

// ---- types -----------------------------------------------------------------

struct rgb_stats_t {
    float r, g, b, mean;
    float center_mean, edge_mean, edge_ratio, radial_span;
    size_t samples;
};

bool collect_rgb_stats(nimg::Pixmap &frame,
                       size_t width,
                       size_t height,
                       const std::vector<unsigned char> &mask,
                       rgb_stats_t &out)
{
    if (mask.size() != width * height) return false;

    double sr = 0.0, sg = 0.0, sb = 0.0;
    double sx = 0.0, sy = 0.0;
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

// ---- base64 ----------------------------------------------------------------

static const char B64_TABLE[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64_encode(const std::vector<unsigned char> &data)
{
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);
    size_t i = 0;
    for (; i + 2 < data.size(); i += 3) {
        uint32_t n = (static_cast<uint32_t>(data[i])   << 16)
                   | (static_cast<uint32_t>(data[i+1]) <<  8)
                   |  static_cast<uint32_t>(data[i+2]);
        out += B64_TABLE[(n >> 18) & 63];
        out += B64_TABLE[(n >> 12) & 63];
        out += B64_TABLE[(n >>  6) & 63];
        out += B64_TABLE[(n      ) & 63];
    }
    if (i + 1 == data.size()) {
        uint32_t n = static_cast<uint32_t>(data[i]) << 16;
        out += B64_TABLE[(n >> 18) & 63]; out += B64_TABLE[(n >> 12) & 63];
        out += '='; out += '=';
    } else if (i + 2 == data.size()) {
        uint32_t n = (static_cast<uint32_t>(data[i]) << 16)
                   | (static_cast<uint32_t>(data[i+1]) << 8);
        out += B64_TABLE[(n >> 18) & 63]; out += B64_TABLE[(n >> 12) & 63];
        out += B64_TABLE[(n >>  6) & 63]; out += '=';
    }
    return out;
}

// ---- integrator factory ----------------------------------------------------

std::unique_ptr<xtcore::render::IIntegrator> make_integrator(const std::string &id)
{
    using namespace xtcore::integrator;
    if (id == "pathtracer")      return std::unique_ptr<xtcore::render::IIntegrator>(new pathtracer::Integrator());
    if (id == "pathtracer_mis")  return std::unique_ptr<xtcore::render::IIntegrator>(new pathtracer_mis::Integrator());
    if (id == "pathtracer_bdpt") return std::unique_ptr<xtcore::render::IIntegrator>(new pathtracer_bdpt::Integrator());
    if (id == "raytracer")       return std::unique_ptr<xtcore::render::IIntegrator>(new raytracer::Integrator());
    if (id == "ao")              return std::unique_ptr<xtcore::render::IIntegrator>(new ao::Integrator());
    return std::unique_ptr<xtcore::render::IIntegrator>();
}

// ---- scene helpers ---------------------------------------------------------

void setup_scene(xtcore::render::context_t &ctx, float albedo)
{
    using xtcore::pool::str::add;

    ctx.params.width      = 40;
    ctx.params.height     = 40;
    ctx.params.tile_size  = 16;
    ctx.params.threads    = 1;
    ctx.params.samples    = 128;
    ctx.params.aa         = 1;
    ctx.params.rdepth     = 8;
    ctx.params.tile_order = xtcore::render::TILE_ORDER_SCANLINE;

    xtcore::camera::Perspective *cam = new xtcore::camera::Perspective();
    cam->position = nmath::Vector3f(0.0f, 0.0f, 0.0f);
    cam->target   = nmath::Vector3f(0.0f, 0.0f, 1.0f);
    cam->up       = nmath::Vector3f(0.0f, 1.0f, 0.0f);
    cam->fov      = 45.0f;
    const HASH_UINT64 cam_id = add("wf_cam");
    ctx.scene->m_cameras[cam_id] = cam;
    ctx.params.camera = cam_id;

    xtcore::surface::Sphere *sphere = new xtcore::surface::Sphere(nmath::Vector3f(0.0f, 0.0f, 3.0f), 1.0f);
    sphere->calc_aabb();
    const HASH_UINT64 geo_id = add("wf_sphere");
    ctx.scene->m_surface[geo_id] = sphere;

    xtcore::asset::material::Lambert *mat = new xtcore::asset::material::Lambert();
    xtcore::sampler::SolidColor *diffuse = new xtcore::sampler::SolidColor();
    nimg::ColorRGBf kd(albedo, albedo, albedo);
    diffuse->set(kd);
    mat->add_sampler("diffuse", diffuse);
    const HASH_UINT64 mat_id = add("wf_lambert");
    ctx.scene->m_materials[mat_id] = mat;

    xtcore::asset::Object *obj = new xtcore::asset::Object();
    obj->surface  = geo_id;
    obj->material = mat_id;
    ctx.scene->m_objects[add("wf_obj")] = obj;

    xtcore::sampler::SolidColor *env = new xtcore::sampler::SolidColor();
    nimg::ColorRGBf env_white(1.0f, 1.0f, 1.0f);
    env->set(env_white);
    ctx.scene->m_environment = env;

    ctx.init();
}

std::vector<unsigned char> build_hit_mask(xtcore::render::context_t &ctx)
{
    std::vector<unsigned char> mask(ctx.params.width * ctx.params.height, 0);
    xtcore::asset::ICamera *cam = ctx.scene->get_camera(ctx.params.camera);
    if (!cam) return mask;
    for (size_t y = 0; y < ctx.params.height; ++y) {
        for (size_t x = 0; x < ctx.params.width; ++x) {
            xtcore::Ray ray = cam->get_primary_ray(
                static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f,
                static_cast<float>(ctx.params.width), static_cast<float>(ctx.params.height));
            xtcore::hit_record_t hr;
            if (ctx.scene->intersection(ray, hr))
                mask[y * ctx.params.width + x] = 1;
        }
    }
    return mask;
}

bool collect_stats(xtcore::render::context_t &ctx,
                   const std::string &integrator_id,
                   const std::vector<unsigned char> &mask,
                   rgb_stats_t &out,
                   std::string *img_b64 = nullptr)
{
    auto integrator = make_integrator(integrator_id);
    if (!integrator) return false;

    integrator->setup(ctx);
    integrator->render();

    nimg::Pixmap frame;
    xtcore::render::assemble(frame, ctx);
    if (!collect_rgb_stats(frame, ctx.params.width, ctx.params.height, mask, out)) return false;

    if (img_b64) {
        xtcore::tonemapping::apply(frame);
        std::vector<unsigned char> png_bytes;
        if (nimg::io::save::png_memory(frame, png_bytes) == 0)
            *img_b64 = "data:image/png;base64," + base64_encode(png_bytes);
    }

    return true;
}

// ---- JSON emitters ---------------------------------------------------------

std::string emit_stats(bool ok, const rgb_stats_t &s, const std::string &img = "")
{
    if (!ok) return "null";
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(6);
    ss << "{\"r\":" << s.r << ",\"g\":" << s.g << ",\"b\":" << s.b
       << ",\"mean\":" << s.mean
       << ",\"center_mean\":" << s.center_mean
       << ",\"edge_mean\":" << s.edge_mean
       << ",\"edge_ratio\":" << s.edge_ratio
       << ",\"radial_span\":" << s.radial_span
       << ",\"samples\":" << s.samples;
    if (!img.empty())
        ss << ",\"img\":\"" << img << "\"";
    ss << "}";
    return ss.str();
}

std::string emit_color(bool ok, const nimg::ColorRGBf &c)
{
    if (!ok) return "null";
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(6);
    ss << "{\"r\":" << c.r() << ",\"g\":" << c.g() << ",\"b\":" << c.b() << "}";
    return ss.str();
}

// ---- per-group runners -----------------------------------------------------

void run_renderer(const std::string &integrator_id, std::ostringstream &json)
{
    rgb_stats_t white, gray;
    std::string img_w, img_g;

    xtcore::render::context_t ctx_w;
    setup_scene(ctx_w, 1.0f);
    auto mask_w = build_hit_mask(ctx_w);
    bool ok_w = collect_stats(ctx_w, integrator_id, mask_w, white, &img_w);

    xtcore::render::context_t ctx_g;
    setup_scene(ctx_g, 0.5f);
    auto mask_g = build_hit_mask(ctx_g);
    bool ok_g = collect_stats(ctx_g, integrator_id, mask_g, gray, &img_g);

    json << "\"white\":" << emit_stats(ok_w, white, img_w)
         << ",\"gray\":"  << emit_stats(ok_g, gray, img_g);
}

void run_rough_dielectric(const std::string &integrator_id, std::ostringstream &json)
{
    using xtcore::pool::str::add;

    auto make_ctx = [&](float roughness, rgb_stats_t &out, bool &ok, std::string &img) {
        xtcore::render::context_t ctx;
        ctx.params.width = 40; ctx.params.height = 40; ctx.params.tile_size = 16;
        ctx.params.threads = 1; ctx.params.samples = 128; ctx.params.aa = 1;
        ctx.params.rdepth = 8; ctx.params.tile_order = xtcore::render::TILE_ORDER_SCANLINE;

        xtcore::camera::Perspective *cam = new xtcore::camera::Perspective();
        cam->position = nmath::Vector3f(0.0f,0.0f,0.0f);
        cam->target   = nmath::Vector3f(0.0f,0.0f,1.0f);
        cam->up       = nmath::Vector3f(0.0f,1.0f,0.0f);
        cam->fov      = 45.0f;
        ctx.scene->m_cameras[add("wf_rd_cam")] = cam;
        ctx.params.camera = add("wf_rd_cam");

        xtcore::surface::Sphere *sphere = new xtcore::surface::Sphere(nmath::Vector3f(0.0f,0.0f,3.0f), 1.0f);
        sphere->calc_aabb();
        ctx.scene->m_surface[add("wf_rd_sphere")] = sphere;

        xtcore::asset::material::RoughDielectric *mat = new xtcore::asset::material::RoughDielectric();
        xtcore::sampler::SolidColor *trans = new xtcore::sampler::SolidColor();
        nimg::ColorRGBf rd_white(1.0f,1.0f,1.0f);
        trans->set(rd_white);
        mat->add_sampler("transmission", trans);
        mat->add_scalar("roughness", roughness);
        mat->add_scalar("ior", 1.5f);
        mat->add_scalar("transparency", 0.98f);
        ctx.scene->m_materials[add("wf_rough_dielectric")] = mat;

        xtcore::asset::Object *obj = new xtcore::asset::Object();
        obj->surface = add("wf_rd_sphere"); obj->material = add("wf_rough_dielectric");
        ctx.scene->m_objects[add("wf_rd_obj")] = obj;

        xtcore::sampler::SolidColor *env = new xtcore::sampler::SolidColor();
        nimg::ColorRGBf rd_env(1.0f,1.0f,1.0f);
        env->set(rd_env);
        ctx.scene->m_environment = env;
        ctx.init();

        auto mask = build_hit_mask(ctx);
        ok = collect_stats(ctx, integrator_id, mask, out, &img);
    };

    rgb_stats_t clear, frosted;
    bool ok_c = false, ok_f = false;
    std::string img_c, img_f;
    make_ctx(0.04f, clear, ok_c, img_c);
    make_ctx(0.28f, frosted, ok_f, img_f);

    json << "\"clear\":"   << emit_stats(ok_c, clear, img_c)
         << ",\"frosted\":" << emit_stats(ok_f, frosted, img_f);
}

void run_absorbing_rough_dielectric(std::ostringstream &json)
{
    xtcore::asset::material::RoughDielectric *mat = new xtcore::asset::material::RoughDielectric();
    xtcore::sampler::SolidColor *trans = new xtcore::sampler::SolidColor();
    xtcore::sampler::SolidColor *absorb = new xtcore::sampler::SolidColor();
    nimg::ColorRGBf ard_white(1.0f,1.0f,1.0f);
    nimg::ColorRGBf ard_blue(0.25f,0.55f,0.92f);
    trans->set(ard_white);
    absorb->set(ard_blue);
    mat->add_sampler("transmission", trans);
    mat->add_sampler("absorption_color", absorb);
    mat->add_scalar("roughness", 0.02f);
    mat->add_scalar("ior", 1.5f);
    mat->add_scalar("transparency", 0.98f);
    mat->add_scalar("absorption_distance", 1.4f);

    xtcore::hit_record_t hit;
    hit.normal             = nmath::Vector3f(0.0f, 0.0f, 1.0f);
    hit.point              = nmath::Vector3f(0.0f, 0.0f, 0.0f);
    hit.texcoord           = nmath::Vector3f(0.5f, 0.5f, 0.0f);
    hit.incident_direction = nmath::Vector3f(0.0f, 0.0f, 1.0f);
    hit.ior                = 1.5f;

    nimg::ColorRGBf absorbed;
    bool found = false;
    for (int i = 0; i < 256; ++i) {
        xtcore::hit_result_t next;
        if (!mat->sample_path(next, hit)) continue;
        if (next.ior < 1.1f) { absorbed = next.intensity; found = true; break; }
    }
    delete mat;

    json << "\"absorbed\":" << emit_color(found, absorbed);
}

void run_principled_clearcoat(const std::string &integrator_id, std::ostringstream &json)
{
    using xtcore::pool::str::add;

    auto make_ctx = [&](float clearcoat, rgb_stats_t &out, bool &ok, std::string &img) {
        xtcore::render::context_t ctx;
        setup_scene(ctx, 1.0f);

        const HASH_UINT64 mat_id = add("wf_lambert");
        xtcore::asset::material::Principled *mat = new xtcore::asset::material::Principled();
        xtcore::sampler::SolidColor *base_color = new xtcore::sampler::SolidColor();
        nimg::ColorRGBf pc_base(0.7f, 0.15f, 0.12f);
        base_color->set(pc_base);
        mat->add_sampler("base_color", base_color);
        mat->add_scalar("metallic", 0.0f);
        mat->add_scalar("roughness", 0.32f);
        mat->add_scalar("ior", 1.5f);
        mat->add_scalar("clearcoat", clearcoat);
        mat->add_scalar("clearcoat_roughness", 0.06f);
        delete ctx.scene->m_materials[mat_id];
        ctx.scene->m_materials[mat_id] = mat;
        ctx.init();

        auto mask = build_hit_mask(ctx);
        ok = collect_stats(ctx, integrator_id, mask, out, &img);
    };

    rgb_stats_t base, coated;
    bool ok_b = false, ok_c = false;
    std::string img_b, img_c;
    make_ctx(0.0f, base, ok_b, img_b);
    make_ctx(1.0f, coated, ok_c, img_c);

    json << "\"base\":"   << emit_stats(ok_b, base, img_b)
         << ",\"coated\":" << emit_stats(ok_c, coated, img_c);
}

void run_principled_anisotropy(const std::string &integrator_id, std::ostringstream &json)
{
    using xtcore::pool::str::add;

    auto make_ctx = [&](float anisotropy, rgb_stats_t &out, bool &ok, std::string &img) {
        xtcore::render::context_t ctx;
        setup_scene(ctx, 1.0f);

        const HASH_UINT64 mat_id = add("wf_lambert");
        xtcore::asset::material::Principled *mat = new xtcore::asset::material::Principled();
        xtcore::sampler::SolidColor *base_color = new xtcore::sampler::SolidColor();
        nimg::ColorRGBf pa_base(0.88f, 0.82f, 0.74f);
        base_color->set(pa_base);
        mat->add_sampler("base_color", base_color);
        mat->add_scalar("metallic", 1.0f);
        mat->add_scalar("roughness", 0.24f);
        mat->add_scalar("anisotropy", anisotropy);
        mat->add_scalar("ior", 1.5f);
        delete ctx.scene->m_materials[mat_id];
        ctx.scene->m_materials[mat_id] = mat;
        ctx.init();

        auto mask = build_hit_mask(ctx);
        ok = collect_stats(ctx, integrator_id, mask, out, &img);
    };

    rgb_stats_t iso, aniso;
    bool ok_i = false, ok_a = false;
    std::string img_i, img_a;
    make_ctx(0.0f,  iso,   ok_i, img_i);
    make_ctx(0.85f, aniso, ok_a, img_a);

    json << "\"isotropic\":"   << emit_stats(ok_i, iso,   img_i)
         << ",\"anisotropic\":" << emit_stats(ok_a, aniso, img_a);
}

void run_thin_dielectric(const std::string &integrator_id, std::ostringstream &json)
{
    using xtcore::pool::str::add;

    auto make_ctx = [&](float roughness, rgb_stats_t &out, bool &ok, std::string &img) {
        xtcore::render::context_t ctx;
        ctx.params.width = 40; ctx.params.height = 40; ctx.params.tile_size = 16;
        ctx.params.threads = 1; ctx.params.samples = 128; ctx.params.aa = 1;
        ctx.params.rdepth = 8; ctx.params.tile_order = xtcore::render::TILE_ORDER_SCANLINE;

        xtcore::camera::Perspective *cam = new xtcore::camera::Perspective();
        cam->position = nmath::Vector3f(0.0f,0.0f,0.0f);
        cam->target   = nmath::Vector3f(0.0f,0.0f,1.0f);
        cam->up       = nmath::Vector3f(0.0f,1.0f,0.0f);
        cam->fov      = 45.0f;
        ctx.scene->m_cameras[add("wf_td_cam")] = cam;
        ctx.params.camera = add("wf_td_cam");

        xtcore::surface::Sphere *sphere = new xtcore::surface::Sphere(nmath::Vector3f(0.0f,0.0f,3.0f), 1.0f);
        sphere->calc_aabb();
        ctx.scene->m_surface[add("wf_td_sphere")] = sphere;

        xtcore::asset::material::ThinDielectric *mat = new xtcore::asset::material::ThinDielectric();
        xtcore::sampler::SolidColor *trans = new xtcore::sampler::SolidColor();
        nimg::ColorRGBf td_white(1.0f,1.0f,1.0f);
        trans->set(td_white);
        mat->add_sampler("transmission", trans);
        mat->add_scalar("roughness", roughness);
        mat->add_scalar("ior", 1.45f);
        mat->add_scalar("transparency", 0.98f);
        ctx.scene->m_materials[add("wf_thin_dielectric")] = mat;

        xtcore::asset::Object *obj = new xtcore::asset::Object();
        obj->surface = add("wf_td_sphere"); obj->material = add("wf_thin_dielectric");
        ctx.scene->m_objects[add("wf_td_obj")] = obj;

        xtcore::sampler::SolidColor *env = new xtcore::sampler::SolidColor();
        nimg::ColorRGBf td_env(1.0f,1.0f,1.0f);
        env->set(td_env);
        ctx.scene->m_environment = env;
        ctx.init();

        auto mask = build_hit_mask(ctx);
        ok = collect_stats(ctx, integrator_id, mask, out, &img);
    };

    rgb_stats_t clear, frosted;
    bool ok_c = false, ok_f = false;
    std::string img_c, img_f;
    make_ctx(0.0f,  clear,   ok_c, img_c);
    make_ctx(0.24f, frosted, ok_f, img_f);

    json << "\"clear\":"   << emit_stats(ok_c, clear,   img_c)
         << ",\"frosted\":" << emit_stats(ok_f, frosted, img_f);
}

void run_subsurface(const std::string &integrator_id, std::ostringstream &json)
{
    using xtcore::pool::str::add;

    auto make_ctx = [&](float subsurface, const nimg::ColorRGBf &radius,
                        float thickness, rgb_stats_t &out, bool &ok, std::string &img) {
        xtcore::render::context_t ctx;
        setup_scene(ctx, 1.0f);

        const HASH_UINT64 mat_id = add("wf_lambert");
        xtcore::asset::material::Subsurface *mat = new xtcore::asset::material::Subsurface();
        xtcore::sampler::SolidColor *base_c    = new xtcore::sampler::SolidColor();
        xtcore::sampler::SolidColor *scatter_c = new xtcore::sampler::SolidColor();
        xtcore::sampler::SolidColor *radius_c  = new xtcore::sampler::SolidColor();
        nimg::ColorRGBf sss_white(1.0f,1.0f,1.0f);
        nimg::ColorRGBf sss_radius(radius.r(), radius.g(), radius.b());
        base_c->set(sss_white);
        scatter_c->set(sss_white);
        radius_c->set(sss_radius);
        mat->add_sampler("base_color",        base_c);
        mat->add_sampler("subsurface_color",  scatter_c);
        mat->add_sampler("subsurface_radius", radius_c);
        mat->add_scalar("subsurface", subsurface);
        mat->add_scalar("thickness",  thickness);
        delete ctx.scene->m_materials[mat_id];
        ctx.scene->m_materials[mat_id] = mat;
        ctx.init();

        auto mask = build_hit_mask(ctx);
        ok = collect_stats(ctx, integrator_id, mask, out, &img);
    };

    const nimg::ColorRGBf rad(0.75f, 1.05f, 1.60f);
    rgb_stats_t mostly_diffuse, thin, thick;
    bool ok_d = false, ok_t = false, ok_k = false;
    std::string img_d, img_t, img_k;
    make_ctx(0.15f, rad, 0.15f, mostly_diffuse, ok_d, img_d);
    make_ctx(0.75f, rad, 0.20f, thin,           ok_t, img_t);
    make_ctx(0.75f, rad, 0.90f, thick,          ok_k, img_k);

    json << "\"mostly_diffuse\":" << emit_stats(ok_d, mostly_diffuse, img_d)
         << ",\"thin\":"           << emit_stats(ok_t, thin,           img_t)
         << ",\"thick\":"          << emit_stats(ok_k, thick,          img_k);
}

void run_sheen(const std::string &integrator_id, std::ostringstream &json)
{
    using xtcore::pool::str::add;

    auto make_ctx = [&](float sheen, rgb_stats_t &out, bool &ok, std::string &img) {
        xtcore::render::context_t ctx;
        setup_scene(ctx, 1.0f);

        const HASH_UINT64 mat_id = add("wf_lambert");
        xtcore::asset::material::Sheen *mat = new xtcore::asset::material::Sheen();
        xtcore::sampler::SolidColor *base_c  = new xtcore::sampler::SolidColor();
        xtcore::sampler::SolidColor *sheen_c = new xtcore::sampler::SolidColor();
        nimg::ColorRGBf sh_base(0.40f,0.28f,0.20f);
        nimg::ColorRGBf sh_tint(0.95f,0.72f,0.55f);
        base_c->set(sh_base);
        sheen_c->set(sh_tint);
        mat->add_sampler("base_color",  base_c);
        mat->add_sampler("sheen_color", sheen_c);
        mat->add_scalar("sheen", sheen);
        delete ctx.scene->m_materials[mat_id];
        ctx.scene->m_materials[mat_id] = mat;
        ctx.init();

        auto mask = build_hit_mask(ctx);
        ok = collect_stats(ctx, integrator_id, mask, out, &img);
    };

    rgb_stats_t low, high;
    bool ok_l = false, ok_h = false;
    std::string img_l, img_h;
    make_ctx(0.05f, low,  ok_l, img_l);
    make_ctx(0.85f, high, ok_h, img_h);

    json << "\"low\":"  << emit_stats(ok_l, low,  img_l)
         << ",\"high\":" << emit_stats(ok_h, high, img_h);
}

void run_thin_translucent(const std::string &integrator_id, std::ostringstream &json)
{
    using xtcore::pool::str::add;

    auto make_ctx = [&](float translucency, float thickness,
                        const nimg::ColorRGBf &tint, rgb_stats_t &out, bool &ok, std::string &img) {
        xtcore::render::context_t ctx;
        setup_scene(ctx, 1.0f);

        const HASH_UINT64 mat_id = add("wf_lambert");
        xtcore::asset::material::ThinTranslucent *mat = new xtcore::asset::material::ThinTranslucent();
        xtcore::sampler::SolidColor *base_c  = new xtcore::sampler::SolidColor();
        xtcore::sampler::SolidColor *trans_c = new xtcore::sampler::SolidColor();
        nimg::ColorRGBf tt_white(1.0f,1.0f,1.0f);
        nimg::ColorRGBf tt_tint(tint.r(), tint.g(), tint.b());
        base_c->set(tt_white);
        trans_c->set(tt_tint);
        mat->add_sampler("base_color",         base_c);
        mat->add_sampler("translucency_color", trans_c);
        mat->add_scalar("translucency", translucency);
        mat->add_scalar("thickness",    thickness);
        delete ctx.scene->m_materials[mat_id];
        ctx.scene->m_materials[mat_id] = mat;
        ctx.init();

        auto mask = build_hit_mask(ctx);
        ok = collect_stats(ctx, integrator_id, mask, out, &img);
    };

    rgb_stats_t low, thin, thick;
    bool ok_l = false, ok_t = false, ok_k = false;
    std::string img_l, img_t, img_k;
    nimg::ColorRGBf tint_low(0.75f, 1.0f, 0.68f);
    nimg::ColorRGBf tint_hi(0.70f, 1.0f, 0.62f);
    make_ctx(0.18f, 0.10f, tint_low, low,   ok_l, img_l);
    make_ctx(0.82f, 0.18f, tint_hi,  thin,  ok_t, img_t);
    make_ctx(0.82f, 0.70f, tint_hi,  thick, ok_k, img_k);

    json << "\"low\":"   << emit_stats(ok_l, low,   img_l)
         << ",\"thin\":"  << emit_stats(ok_t, thin,  img_t)
         << ",\"thick\":" << emit_stats(ok_k, thick, img_k);
}

} // anonymous namespace

// ---- public API ------------------------------------------------------------

std::string run_furnace_group(const std::string &group, const std::string &integrator)
{
    xtcore::Log::handle().echo(false);

    std::ostringstream cases;
    bool known = true;

    if      (group == "renderer")                   run_renderer(integrator, cases);
    else if (group == "thin_dielectric")            run_thin_dielectric(integrator, cases);
    else if (group == "rough_dielectric")           run_rough_dielectric(integrator, cases);
    else if (group == "absorbing_rough_dielectric") run_absorbing_rough_dielectric(cases);
    else if (group == "principled_clearcoat")       run_principled_clearcoat(integrator, cases);
    else if (group == "principled_anisotropy")      run_principled_anisotropy(integrator, cases);
    else if (group == "subsurface")                 run_subsurface(integrator, cases);
    else if (group == "sheen")                      run_sheen(integrator, cases);
    else if (group == "thin_translucent")           run_thin_translucent(integrator, cases);
    else                                            known = false;

    if (!known)
        return "{\"error\":\"unknown group\"}";

    std::ostringstream json;
    json << "{\"group\":\"" << group << "\","
         << "\"integrator\":\"" << integrator << "\","
         << "\"cases\":{" << cases.str() << "}}";
    return json.str();
}

} // web
} // frontend
} // xtracer
