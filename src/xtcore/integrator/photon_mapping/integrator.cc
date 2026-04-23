#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <map>
#include <utility>
#include <vector>

#include <nimg/luminance.h>
#include <nmath/prng.h>
#include <xtcore/aa.h>
#include <xtcore/matdefs.h>
#include <xtcore/math/sampling_util.h>
#include <xtcore/math/sphere.h>
#include <xtcore/math/triangle.h>
#include <xtcore/mesh.h>
#include <xtcore/tile.h>

#include "integrator.h"

namespace xtcore {
namespace integrator {
namespace photon_mapping {

namespace {

struct photon_candidate_t
{
    nmath::scalar_t dist2;
    const Integrator::Photon *photon;
};

inline bool is_finite_scalar(nmath::scalar_t v)
{
    return std::isfinite((double)v) != 0;
}

inline bool is_finite_color(const nimg::ColorRGBf &c)
{
    return is_finite_scalar(c.r()) && is_finite_scalar(c.g()) && is_finite_scalar(c.b());
}

inline nmath::scalar_t saturate(nmath::scalar_t v)
{
    if (v < (nmath::scalar_t)0.0) return (nmath::scalar_t)0.0;
    if (v > (nmath::scalar_t)1.0) return (nmath::scalar_t)1.0;
    return v;
}

inline nmath::Vector3f sample_uniform_cone(const nmath::Vector3f &axis,
                                           nmath::scalar_t cos_theta_max,
                                           nmath::scalar_t &pdf)
{
    const nmath::scalar_t u1 = nmath::prng_c(0.0, 1.0);
    const nmath::scalar_t u2 = nmath::prng_c(0.0, 1.0);

    const nmath::scalar_t cos_theta = (nmath::scalar_t)1.0 - u1 * ((nmath::scalar_t)1.0 - cos_theta_max);
    const nmath::scalar_t sin_theta = nmath_sqrt(std::max((nmath::scalar_t)0.0, (nmath::scalar_t)1.0 - cos_theta * cos_theta));
    const nmath::scalar_t phi = nmath::PI_DOUBLE * u2;

    const nmath::scalar_t x = sin_theta * nmath_cos(phi);
    const nmath::scalar_t z = sin_theta * nmath_sin(phi);
    const nmath::scalar_t y = cos_theta;

    const nmath::Vector3f n = axis.normalized();
    const nmath::Vector3f t = xtcore::math::sampling::build_tangent(n);
    const nmath::Vector3f b = nmath::cross(n, t).normalized();

    nmath::Vector3f dir = (t * x) + (n * y) + (b * z);
    dir.normalize();

    const nmath::scalar_t solid = nmath::PI_DOUBLE * (nmath::scalar_t)2.0 * ((nmath::scalar_t)1.0 - cos_theta_max);
    pdf = (solid > (nmath::scalar_t)EPSILON) ? ((nmath::scalar_t)1.0 / solid) : (nmath::scalar_t)0.0;
    return dir;
}

inline nmath::scalar_t triangle_area(const nmath::Vector3f &a, const nmath::Vector3f &b, const nmath::Vector3f &c)
{
    return (nmath::scalar_t)0.5 * nmath::cross(b - a, c - a).length();
}

inline nmath::scalar_t surface_area(const xtcore::asset::ISurface *surface)
{
    if (!surface) return 0.0;

    const xtcore::surface::Sphere *sp = dynamic_cast<const xtcore::surface::Sphere *>(surface);
    if (sp) return (nmath::scalar_t)(4.0 * nmath::PI * sp->radius * sp->radius);

    const xtcore::surface::Triangle *tr = dynamic_cast<const xtcore::surface::Triangle *>(surface);
    if (tr) return triangle_area(tr->v[0], tr->v[1], tr->v[2]);

    const xtcore::surface::Mesh *mesh = dynamic_cast<const xtcore::surface::Mesh *>(surface);
    if (mesh) {
        const std::vector<xtcore::surface::Triangle> &tris = mesh->triangles();
        nmath::scalar_t sum = 0.0;
        for (size_t i = 0; i < tris.size(); ++i) {
            sum += triangle_area(tris[i].v[0], tris[i].v[1], tris[i].v[2]);
        }
        return sum;
    }

    return 0.0;
}

inline bool sample_barycentric(nmath::scalar_t &b0, nmath::scalar_t &b1, nmath::scalar_t &b2)
{
    b0 = nmath::prng_c(0.0, 1.0);
    b1 = nmath::prng_c(0.0, 1.0);
    b2 = nmath::prng_c(0.0, 1.0);

    const nmath::scalar_t sum = b0 + b1 + b2;
    if (sum <= (nmath::scalar_t)EPSILON) return false;

    b0 /= sum;
    b1 /= sum;
    b2 /= sum;
    return true;
}

inline bool sample_light_point(const Integrator::AreaLight &light,
                               nmath::Vector3f &point,
                               nmath::Vector3f &normal,
                               nmath::Vector3f &texcoord,
                               nmath::scalar_t &pdf_area)
{
    if (!light.surface || light.area <= (nmath::scalar_t)EPSILON) return false;

    const xtcore::surface::Sphere *sp = dynamic_cast<const xtcore::surface::Sphere *>(light.surface);
    if (sp) {
        point = sp->point_sample();
        normal = (point - sp->origin).normalized();

        const nmath::scalar_t sx = (sp->uv_scale.x != 0.0f) ? sp->uv_scale.x : 1.0f;
        const nmath::scalar_t sy = (sp->uv_scale.y != 0.0f) ? sp->uv_scale.y : 1.0f;
        texcoord = nmath::Vector3f(
            (nmath_asin(normal.x / sx) / nmath::PI + 0.5),
            (nmath_asin(normal.y / sy) / nmath::PI + 0.5),
            0.0
        );

        pdf_area = 1.0 / light.area;
        return true;
    }

    const xtcore::surface::Triangle *tr = dynamic_cast<const xtcore::surface::Triangle *>(light.surface);
    if (tr) {
        nmath::scalar_t b0, b1, b2;
        if (!sample_barycentric(b0, b1, b2)) return false;

        point = tr->v[0] * b0 + tr->v[1] * b1 + tr->v[2] * b2;
        normal = tr->n[0] * b0 + tr->n[1] * b1 + tr->n[2] * b2;
        if (normal.length() <= (nmath::scalar_t)EPSILON) normal = tr->calc_normal();
        normal.normalize();

        const nmath::Vector2f uv = tr->tc[0] * b0 + tr->tc[1] * b1 + tr->tc[2] * b2;
        texcoord = nmath::Vector3f(uv.x, uv.y, 0.0);
        pdf_area = 1.0 / light.area;
        return true;
    }

    const xtcore::surface::Mesh *mesh = dynamic_cast<const xtcore::surface::Mesh *>(light.surface);
    if (mesh) {
        const std::vector<xtcore::surface::Triangle> &tris = mesh->triangles();
        if (tris.empty()) return false;

        const nmath::scalar_t pick = nmath::prng_c(0.0, 1.0) * light.area;
        nmath::scalar_t accum = 0.0;
        const xtcore::surface::Triangle *selected = &tris[0];

        for (size_t i = 0; i < tris.size(); ++i) {
            const nmath::scalar_t a = triangle_area(tris[i].v[0], tris[i].v[1], tris[i].v[2]);
            if (a <= (nmath::scalar_t)EPSILON) continue;
            accum += a;
            if (pick <= accum) {
                selected = &tris[i];
                break;
            }
        }

        nmath::scalar_t b0, b1, b2;
        if (!sample_barycentric(b0, b1, b2)) return false;

        point = selected->v[0] * b0 + selected->v[1] * b1 + selected->v[2] * b2;
        normal = selected->n[0] * b0 + selected->n[1] * b1 + selected->n[2] * b2;
        if (normal.length() <= (nmath::scalar_t)EPSILON) normal = selected->calc_normal();
        normal.normalize();

        const nmath::Vector2f uv = selected->tc[0] * b0 + selected->tc[1] * b1 + selected->tc[2] * b2;
        const nmath::scalar_t sx = (mesh->uv_scale.x != 0.0f) ? mesh->uv_scale.x : 1.0f;
        const nmath::scalar_t sy = (mesh->uv_scale.y != 0.0f) ? mesh->uv_scale.y : 1.0f;
        texcoord = nmath::Vector3f(uv.x * sx, uv.y * sy, 0.0);
        pdf_area = 1.0 / light.area;
        return true;
    }

    return false;
}

inline bool sample_surface_target(const xtcore::asset::ISurface *surface,
                                  nmath::Vector3f &point)
{
    if (!surface) return false;

    if (is_finite_scalar(surface->aabb.min.x) && is_finite_scalar(surface->aabb.min.y) && is_finite_scalar(surface->aabb.min.z) &&
        is_finite_scalar(surface->aabb.max.x) && is_finite_scalar(surface->aabb.max.y) && is_finite_scalar(surface->aabb.max.z)) {
        point = (surface->aabb.min + surface->aabb.max) * (nmath::scalar_t)0.5;
        return true;
    }

    const xtcore::surface::Sphere *sp = dynamic_cast<const xtcore::surface::Sphere *>(surface);
    if (sp) {
        point = sp->origin;
        return true;
    }

    const xtcore::surface::Triangle *tr = dynamic_cast<const xtcore::surface::Triangle *>(surface);
    if (tr) {
        point = (tr->v[0] + tr->v[1] + tr->v[2]) / (nmath::scalar_t)3.0;
        return true;
    }

    const xtcore::surface::Mesh *mesh = dynamic_cast<const xtcore::surface::Mesh *>(surface);
    if (mesh) {
        point = mesh->emitter_position();
        return true;
    }

    return false;
}

inline bool is_diffuse_like(const xtcore::asset::IMaterial *mat, const xtcore::hit_record_t &hit)
{
    if (!mat) return false;
    const nimg::ColorRGBf kd = mat->get_sample(MAT_SAMPLER_DIFFUSE, hit.texcoord);
    return nimg::eval::luminance(kd) > (nmath::scalar_t)EPSILON;
}

} // namespace

Integrator::Integrator()
    : m_scene_diag(10.0)
    , m_gather_radius(0.25)
    , m_caustic_gather_radius(0.125)
    , m_gather_k(64)
    , m_caustic_gather_k(32)
    , m_emit_photons(20000)
    , m_caustic_emit_photons(20000)
    , m_override_gather_radius(false)
    , m_override_caustic_gather_radius(false)
    , m_override_gather_k(false)
    , m_override_caustic_gather_k(false)
    , m_override_emit_photons(false)
    , m_override_caustic_emit_photons(false)
    , m_config_gather_radius(0.25)
    , m_config_caustic_gather_radius(0.125)
    , m_config_gather_k(64)
    , m_config_caustic_gather_k(32)
    , m_config_emit_photons(20000)
    , m_config_caustic_emit_photons(20000)
{}

void Integrator::configure(const std::map<std::string, std::string> &options)
{
    m_override_emit_photons = false;
    m_override_caustic_emit_photons = false;
    m_override_gather_radius = false;
    m_override_caustic_gather_radius = false;
    m_override_gather_k = false;
    m_override_caustic_gather_k = false;

    auto it_emit = options.find("emit_photons");
    if (it_emit != options.end()) {
        char *end = nullptr;
        unsigned long long v = std::strtoull(it_emit->second.c_str(), &end, 10);
        if (!(end == it_emit->second.c_str() || !end || *end != '\0') && v > 0) {
            m_config_emit_photons = (size_t)v;
            m_override_emit_photons = true;
        }
    }

    auto it_radius = options.find("gather_radius");
    if (it_radius != options.end()) {
        char *end = nullptr;
        double v = std::strtod(it_radius->second.c_str(), &end);
        if (!(end == it_radius->second.c_str() || !end || *end != '\0') && v > 0.0) {
            m_config_gather_radius = (nmath::scalar_t)v;
            m_override_gather_radius = true;
        }
    }

    auto it_caustic_radius = options.find("caustic_gather_radius");
    if (it_caustic_radius != options.end()) {
        char *end = nullptr;
        double v = std::strtod(it_caustic_radius->second.c_str(), &end);
        if (!(end == it_caustic_radius->second.c_str() || !end || *end != '\0') && v > 0.0) {
            m_config_caustic_gather_radius = (nmath::scalar_t)v;
            m_override_caustic_gather_radius = true;
        }
    }

    auto it_k = options.find("gather_k");
    if (it_k != options.end()) {
        char *end = nullptr;
        unsigned long long v = std::strtoull(it_k->second.c_str(), &end, 10);
        if (!(end == it_k->second.c_str() || !end || *end != '\0') && v > 0) {
            m_config_gather_k = (size_t)v;
            m_override_gather_k = true;
        }
    }

    auto it_caustic_k = options.find("caustic_gather_k");
    if (it_caustic_k != options.end()) {
        char *end = nullptr;
        unsigned long long v = std::strtoull(it_caustic_k->second.c_str(), &end, 10);
        if (!(end == it_caustic_k->second.c_str() || !end || *end != '\0') && v > 0) {
            m_config_caustic_gather_k = (size_t)v;
            m_override_caustic_gather_k = true;
        }
    }

    auto it_caustic_emit = options.find("caustic_emit_photons");
    if (it_caustic_emit != options.end()) {
        char *end = nullptr;
        unsigned long long v = std::strtoull(it_caustic_emit->second.c_str(), &end, 10);
        if (!(end == it_caustic_emit->second.c_str() || !end || *end != '\0') && v > 0) {
            m_config_caustic_emit_photons = (size_t)v;
            m_override_caustic_emit_photons = true;
        }
    }
}

void Integrator::setup_auxiliary()
{
    build_photon_map();
}

void Integrator::clean_auxiliary()
{
    m_global_photons.clear();
    m_caustic_photons.clear();
    m_global_map.clear();
    m_caustic_map.clear();
    m_lights.clear();
    m_caustic_guides.clear();
}

const std::vector<nmath::Vector3f> &Integrator::debug_global_points() const
{
    return m_debug_global_points;
}

const std::vector<nmath::Vector3f> &Integrator::debug_caustic_points() const
{
    return m_debug_caustic_points;
}

void Integrator::trace_photon(const xtcore::Ray &in_ray,
                              const nimg::ColorRGBf &in_power,
                              nmath::scalar_t in_ior,
                              size_t depth,
                              bool has_specular_bounce,
                              bool collect_global,
                              bool collect_caustic)
{
    if (depth == 0) return;

    xtcore::Ray ray = in_ray;
    nimg::ColorRGBf power = in_power;
    nmath::scalar_t ior = in_ior;

    for (size_t bounce = 0; bounce < depth; ++bounce) {
        if (!is_finite_color(power) ||
            power.r() < (nmath::scalar_t)0.0 ||
            power.g() < (nmath::scalar_t)0.0 ||
            power.b() < (nmath::scalar_t)0.0) {
            return;
        }

        xtcore::hit_record_t hit;
        if (!ctx->scene.intersection(ray, hit)) return;
        hit.ior = ior;

        const xtcore::asset::IMaterial *mat = ctx->scene.get_material(hit.id_object);
        if (!mat) return;

        if (mat->is_emissive()) return;

        if (is_diffuse_like(mat, hit)) {
            Photon ph;
            ph.position = hit.point;
            ph.normal = hit.normal.normalized();
            ph.incident = -ray.direction.normalized();
            ph.power = power;

            if (collect_global) {
                m_global_photons.push_back(ph);
            }
            if (collect_caustic && has_specular_bounce) {
                m_caustic_photons.push_back(ph);
            }

            nmath::scalar_t pdf = 0.0;
            const nmath::Vector3f wo = xtcore::math::sampling::sample_cosine_hemisphere(hit.normal, pdf);
            if (pdf <= (nmath::scalar_t)EPSILON) return;

            const nimg::ColorRGBf kd = mat->get_sample(MAT_SAMPLER_DIFFUSE, hit.texcoord);
            power *= kd;

            ray.origin = hit.point + hit.normal * EPSILON;
            ray.direction = wo;
            has_specular_bounce = false;
        } else {
            hit_result_t next_hit;
            next_hit.ior = ior;
            const bool cont = mat->sample_path(next_hit, hit);
            power *= next_hit.intensity;
            ray = next_hit.ray;
            ior = next_hit.ior;
            if (!cont) return;
            has_specular_bounce = true;
        }

        if (bounce >= 3) {
            nmath::scalar_t rr = nimg::eval::luminance(power);
            rr = std::max((nmath::scalar_t)0.05, std::min((nmath::scalar_t)0.95, rr));
            if (nmath::prng_c(0.0, 1.0) > rr) return;
            power *= (1.0 / rr);
        }
    }
}

void Integrator::build_photon_map()
{
    m_global_photons.clear();
    m_caustic_photons.clear();
    m_debug_global_points.clear();
    m_debug_caustic_points.clear();
    m_global_map.clear();
    m_caustic_map.clear();
    m_lights.clear();
    m_caustic_guides.clear();

    nmath::Vector3f bmin( std::numeric_limits<nmath::scalar_t>::max(),
                          std::numeric_limits<nmath::scalar_t>::max(),
                          std::numeric_limits<nmath::scalar_t>::max());
    nmath::Vector3f bmax(-std::numeric_limits<nmath::scalar_t>::max(),
                         -std::numeric_limits<nmath::scalar_t>::max(),
                         -std::numeric_limits<nmath::scalar_t>::max());
    bool has_bounds = false;

    nmath::scalar_t cdf = 0.0;
    nmath::scalar_t guide_cdf = 0.0;
    for (auto it = ctx->scene.m_objects.begin(); it != ctx->scene.m_objects.end(); ++it) {
        const HASH_ID obj_id = (*it).first;
        const xtcore::asset::Object *obj = (*it).second;
        if (!obj) continue;

        auto sit = ctx->scene.m_surface.find(obj->surface);
        auto mit = ctx->scene.m_materials.find(obj->material);
        if (sit == ctx->scene.m_surface.end() || mit == ctx->scene.m_materials.end()) continue;

        const xtcore::asset::ISurface *surface = (*sit).second;
        const xtcore::asset::IMaterial *material = (*mit).second;
        if (!surface || !material) continue;

        if (is_finite_scalar(surface->aabb.min.x) && is_finite_scalar(surface->aabb.min.y) && is_finite_scalar(surface->aabb.min.z) &&
            is_finite_scalar(surface->aabb.max.x) && is_finite_scalar(surface->aabb.max.y) && is_finite_scalar(surface->aabb.max.z)) {
            bmin.x = std::min(bmin.x, surface->aabb.min.x);
            bmin.y = std::min(bmin.y, surface->aabb.min.y);
            bmin.z = std::min(bmin.z, surface->aabb.min.z);
            bmax.x = std::max(bmax.x, surface->aabb.max.x);
            bmax.y = std::max(bmax.y, surface->aabb.max.y);
            bmax.z = std::max(bmax.z, surface->aabb.max.z);
            has_bounds = true;
        }

        if (!material->is_emissive()) {
            const nmath::scalar_t area = surface_area(surface);
            if (area > (nmath::scalar_t)EPSILON) {
                const nimg::ColorRGBf kd = material->get_sample(MAT_SAMPLER_DIFFUSE, nmath::Vector3f(0, 0, 0));
                const nimg::ColorRGBf ks = material->get_sample(MAT_SAMPLER_SPECULAR, nmath::Vector3f(0, 0, 0));
                nmath::scalar_t reflectance = material->get_scalar("reflectance");
                if (!is_finite_scalar(reflectance)) reflectance = 0.0;
                reflectance = std::max((nmath::scalar_t)0.0, std::min((nmath::scalar_t)1.0, reflectance));

                const nmath::scalar_t spec_luma = nimg::eval::luminance(ks);
                const nmath::scalar_t diff_luma = nimg::eval::luminance(kd);
                const nmath::scalar_t guide_ratio = std::max((nmath::scalar_t)0.0, (spec_luma + reflectance) - diff_luma * (nmath::scalar_t)0.5);
                const nmath::scalar_t guide_weight = area * guide_ratio;
                if (guide_weight > (nmath::scalar_t)EPSILON) {
                    guide_cdf += guide_weight;
                    CausticGuide guide;
                    guide.surface = surface;
                    guide.weight_cdf = guide_cdf;
                    m_caustic_guides.push_back(guide);
                }
            }
            continue;
        }

        const nmath::scalar_t area = surface_area(surface);
        if (area <= (nmath::scalar_t)EPSILON) continue;

        const nimg::ColorRGBf le = material->get_sample(MAT_SAMPLER_EMISSIVE, nmath::Vector3f(0, 0, 0));
        nmath::scalar_t weight = nimg::eval::luminance(le) * area;
        if (weight <= (nmath::scalar_t)EPSILON) weight = area;

        cdf += weight;

        AreaLight light;
        light.object_id = obj_id;
        light.surface = surface;
        light.material = material;
        light.area = area;
        light.weight_cdf = cdf;
        m_lights.push_back(light);
    }

    m_scene_diag = has_bounds ? (bmax - bmin).length() : (nmath::scalar_t)10.0;
    if (m_scene_diag <= (nmath::scalar_t)EPSILON) m_scene_diag = (nmath::scalar_t)10.0;

    m_gather_radius = std::max((nmath::scalar_t)0.05, m_scene_diag * (nmath::scalar_t)0.02);
    if (m_override_gather_radius) m_gather_radius = std::max((nmath::scalar_t)0.001, m_config_gather_radius);
    m_caustic_gather_radius = std::max((nmath::scalar_t)0.001, m_gather_radius * (nmath::scalar_t)0.5);
    if (m_override_caustic_gather_radius) {
        m_caustic_gather_radius = std::max((nmath::scalar_t)0.001, m_config_caustic_gather_radius);
    }

    m_gather_k = m_override_gather_k ? std::max((size_t)1, m_config_gather_k) : (size_t)64;
    m_caustic_gather_k = m_override_caustic_gather_k
        ? std::max((size_t)1, m_config_caustic_gather_k)
        : std::max((size_t)8, m_gather_k / (size_t)2);

    m_emit_photons = std::max((size_t)20000, ctx->params.samples * (size_t)8000);
    m_emit_photons = std::min((size_t)120000, m_emit_photons);
    if (m_override_emit_photons) m_emit_photons = std::max((size_t)1, m_config_emit_photons);
    m_caustic_emit_photons = m_override_caustic_emit_photons
        ? std::max((size_t)1, m_config_caustic_emit_photons)
        : std::max((size_t)1, m_emit_photons);

    if (m_lights.empty() || m_emit_photons == 0) return;

    m_global_photons.reserve(m_emit_photons);
    m_caustic_photons.reserve(m_caustic_emit_photons / 2);

    const nmath::scalar_t total_cdf = m_lights.back().weight_cdf;
    const nmath::scalar_t inv_emit = 1.0 / (nmath::scalar_t)m_emit_photons;
    const xtcore::asset::ICamera *cam = ctx->active_camera();
    const nmath::Vector3f cam_pos = cam ? cam->position : nmath::Vector3f(0.0, 0.0, 0.0);
    const bool use_camera_guiding = (cam != nullptr);
    const nmath::scalar_t guide_mix = (nmath::scalar_t)0.6;
    const nmath::scalar_t guide_cone_deg = (nmath::scalar_t)25.0;
    const nmath::scalar_t guide_cos_max = nmath_cos(guide_cone_deg * nmath::RADIAN);
    const nmath::scalar_t caustic_spec_guide_mix = (nmath::scalar_t)0.9;
    const nmath::scalar_t caustic_spec_guide_cone_deg = (nmath::scalar_t)2.5;
    const nmath::scalar_t caustic_spec_guide_cos_max = nmath_cos(caustic_spec_guide_cone_deg * nmath::RADIAN);
    const nmath::scalar_t caustic_guide_total_cdf = m_caustic_guides.empty() ? (nmath::scalar_t)0.0 : m_caustic_guides.back().weight_cdf;

    for (size_t i = 0; i < m_emit_photons; ++i) {
        const nmath::scalar_t u = nmath::prng_c(0.0, 1.0) * total_cdf;
        size_t light_idx = 0;
        while (light_idx + 1 < m_lights.size() && m_lights[light_idx].weight_cdf < u) ++light_idx;

        const AreaLight &light = m_lights[light_idx];
        const nmath::scalar_t prev_cdf = (light_idx == 0) ? (nmath::scalar_t)0.0 : m_lights[light_idx - 1].weight_cdf;
        const nmath::scalar_t light_prob = (light.weight_cdf - prev_cdf) / total_cdf;
        if (light_prob <= (nmath::scalar_t)EPSILON) continue;

        nmath::Vector3f lp, ln, ltc;
        nmath::scalar_t pdf_area = 0.0;
        if (!sample_light_point(light, lp, ln, ltc, pdf_area)) continue;
        if (pdf_area <= (nmath::scalar_t)EPSILON) continue;

        const nmath::Vector3f to_cam = cam_pos - lp;
        nmath::Vector3f dir;
        nmath::scalar_t pdf_dir = 0.0;
        bool guide_active = false;

        if (use_camera_guiding) {
            const nmath::scalar_t to_cam_len2 = to_cam.length_squared();
            if (to_cam_len2 > (nmath::scalar_t)EPSILON) {
                const nmath::Vector3f axis = to_cam / nmath_sqrt(to_cam_len2);
                const nmath::scalar_t align = nmath::dot(axis, ln);
                // Keep the guide cone fully in the emissive hemisphere to keep PDF simple.
                if (align > guide_cos_max + (nmath::scalar_t)1e-4) {
                    guide_active = true;
                    if (nmath::prng_c(0.0, 1.0) < guide_mix) {
                        nmath::scalar_t cone_pdf = 0.0;
                        dir = sample_uniform_cone(axis, guide_cos_max, cone_pdf);
                    } else {
                        nmath::scalar_t cos_pdf = 0.0;
                        dir = xtcore::math::sampling::sample_cosine_hemisphere(ln, cos_pdf);
                    }

                    const nmath::scalar_t cos_l_mix = std::max((nmath::scalar_t)0.0, nmath::dot(ln, dir));
                    const nmath::scalar_t pdf_cos = cos_l_mix / nmath::PI;
                    const nmath::scalar_t cone_pdf_const =
                        ((nmath::PI_DOUBLE * (nmath::scalar_t)2.0 * ((nmath::scalar_t)1.0 - guide_cos_max)) > (nmath::scalar_t)EPSILON)
                        ? ((nmath::scalar_t)1.0 / (nmath::PI_DOUBLE * (nmath::scalar_t)2.0 * ((nmath::scalar_t)1.0 - guide_cos_max)))
                        : (nmath::scalar_t)0.0;
                    const bool in_cone = nmath::dot(dir, axis) >= guide_cos_max;
                    pdf_dir = (nmath::scalar_t)(1.0 - guide_mix) * pdf_cos + (in_cone ? (guide_mix * cone_pdf_const) : (nmath::scalar_t)0.0);
                }
            }
        }

        if (!guide_active) {
            dir = xtcore::math::sampling::sample_cosine_hemisphere(ln, pdf_dir);
        }
        if (pdf_dir <= (nmath::scalar_t)EPSILON) continue;

        const nimg::ColorRGBf le = light.material->get_sample(MAT_SAMPLER_EMISSIVE, ltc);
        const nmath::scalar_t cos_l = std::max((nmath::scalar_t)0.0, nmath::dot(ln, dir));
        if (cos_l <= (nmath::scalar_t)EPSILON) continue;

        const nimg::ColorRGBf power = le * (cos_l / (light_prob * pdf_area * pdf_dir)) * inv_emit;

        xtcore::Ray ray;
        ray.origin = lp + ln * EPSILON;
        ray.direction = dir;

        trace_photon(ray, power, 1.0, ctx->params.rdepth, false, true, false);
    }

    if (m_caustic_emit_photons > 0) {
        const nmath::scalar_t inv_emit_caustic = 1.0 / (nmath::scalar_t)m_caustic_emit_photons;
        for (size_t i = 0; i < m_caustic_emit_photons; ++i) {
            const nmath::scalar_t u = nmath::prng_c(0.0, 1.0) * total_cdf;
            size_t light_idx = 0;
            while (light_idx + 1 < m_lights.size() && m_lights[light_idx].weight_cdf < u) ++light_idx;

            const AreaLight &light = m_lights[light_idx];
            const nmath::scalar_t prev_cdf = (light_idx == 0) ? (nmath::scalar_t)0.0 : m_lights[light_idx - 1].weight_cdf;
            const nmath::scalar_t light_prob = (light.weight_cdf - prev_cdf) / total_cdf;
            if (light_prob <= (nmath::scalar_t)EPSILON) continue;

            nmath::Vector3f lp, ln, ltc;
            nmath::scalar_t pdf_area = 0.0;
            if (!sample_light_point(light, lp, ln, ltc, pdf_area)) continue;
            if (pdf_area <= (nmath::scalar_t)EPSILON) continue;

            nmath::Vector3f dir;
            nmath::scalar_t pdf_dir = 0.0;
            bool guide_active = false;

            if (!m_caustic_guides.empty()
                && caustic_guide_total_cdf > (nmath::scalar_t)EPSILON
                && nmath::prng_c(0.0, 1.0) < caustic_spec_guide_mix) {
                const nmath::scalar_t ug = nmath::prng_c(0.0, 1.0) * caustic_guide_total_cdf;
                size_t guide_idx = 0;
                while (guide_idx + 1 < m_caustic_guides.size() && m_caustic_guides[guide_idx].weight_cdf < ug) ++guide_idx;
                const CausticGuide &guide = m_caustic_guides[guide_idx];
                const nmath::scalar_t guide_prev = (guide_idx == 0) ? (nmath::scalar_t)0.0 : m_caustic_guides[guide_idx - 1].weight_cdf;
                const nmath::scalar_t guide_prob = (guide.weight_cdf - guide_prev) / caustic_guide_total_cdf;

                nmath::Vector3f gp;
                if (guide_prob > (nmath::scalar_t)EPSILON && sample_surface_target(guide.surface, gp)) {
                    nmath::Vector3f axis = gp - lp;
                    const nmath::scalar_t axis_len2 = axis.length_squared();
                    if (axis_len2 > (nmath::scalar_t)EPSILON) {
                        axis /= nmath_sqrt(axis_len2);
                        const nmath::scalar_t align = nmath::dot(axis, ln);
                        if (align > caustic_spec_guide_cos_max + (nmath::scalar_t)1e-4) {
                            nmath::scalar_t cone_pdf = 0.0;
                            dir = sample_uniform_cone(axis, caustic_spec_guide_cos_max, cone_pdf);
                            const nmath::scalar_t cos_l_mix = std::max((nmath::scalar_t)0.0, nmath::dot(ln, dir));
                            const nmath::scalar_t pdf_cos = cos_l_mix / nmath::PI;
                            const nmath::scalar_t cone_pdf_const =
                                ((nmath::PI_DOUBLE * (nmath::scalar_t)2.0 * ((nmath::scalar_t)1.0 - caustic_spec_guide_cos_max)) > (nmath::scalar_t)EPSILON)
                                ? ((nmath::scalar_t)1.0 / (nmath::PI_DOUBLE * (nmath::scalar_t)2.0 * ((nmath::scalar_t)1.0 - caustic_spec_guide_cos_max)))
                                : (nmath::scalar_t)0.0;
                            const bool in_cone = nmath::dot(dir, axis) >= caustic_spec_guide_cos_max;
                            const nmath::scalar_t pdf_spec = in_cone ? (guide_prob * cone_pdf_const) : (nmath::scalar_t)0.0;
                            pdf_dir = ((nmath::scalar_t)1.0 - caustic_spec_guide_mix) * pdf_cos + caustic_spec_guide_mix * pdf_spec;
                            guide_active = (pdf_dir > (nmath::scalar_t)EPSILON);
                        }
                    }
                }
            }

            const nmath::Vector3f to_cam = cam_pos - lp;
            if (!guide_active && use_camera_guiding) {
                const nmath::scalar_t to_cam_len2 = to_cam.length_squared();
                if (to_cam_len2 > (nmath::scalar_t)EPSILON) {
                    const nmath::Vector3f axis = to_cam / nmath_sqrt(to_cam_len2);
                    const nmath::scalar_t align = nmath::dot(axis, ln);
                    if (align > guide_cos_max + (nmath::scalar_t)1e-4) {
                        guide_active = true;
                        if (nmath::prng_c(0.0, 1.0) < guide_mix) {
                            nmath::scalar_t cone_pdf = 0.0;
                            dir = sample_uniform_cone(axis, guide_cos_max, cone_pdf);
                        } else {
                            nmath::scalar_t cos_pdf = 0.0;
                            dir = xtcore::math::sampling::sample_cosine_hemisphere(ln, cos_pdf);
                        }

                        const nmath::scalar_t cos_l_mix = std::max((nmath::scalar_t)0.0, nmath::dot(ln, dir));
                        const nmath::scalar_t pdf_cos = cos_l_mix / nmath::PI;
                        const nmath::scalar_t cone_pdf_const =
                            ((nmath::PI_DOUBLE * (nmath::scalar_t)2.0 * ((nmath::scalar_t)1.0 - guide_cos_max)) > (nmath::scalar_t)EPSILON)
                            ? ((nmath::scalar_t)1.0 / (nmath::PI_DOUBLE * (nmath::scalar_t)2.0 * ((nmath::scalar_t)1.0 - guide_cos_max)))
                            : (nmath::scalar_t)0.0;
                        const bool in_cone = nmath::dot(dir, axis) >= guide_cos_max;
                        pdf_dir = (nmath::scalar_t)(1.0 - guide_mix) * pdf_cos + (in_cone ? (guide_mix * cone_pdf_const) : (nmath::scalar_t)0.0);
                    }
                }
            }

            if (!guide_active) {
                dir = xtcore::math::sampling::sample_cosine_hemisphere(ln, pdf_dir);
            }
            if (pdf_dir <= (nmath::scalar_t)EPSILON) continue;

            const nimg::ColorRGBf le = light.material->get_sample(MAT_SAMPLER_EMISSIVE, ltc);
            const nmath::scalar_t cos_l = std::max((nmath::scalar_t)0.0, nmath::dot(ln, dir));
            if (cos_l <= (nmath::scalar_t)EPSILON) continue;

            const nimg::ColorRGBf power = le * (cos_l / (light_prob * pdf_area * pdf_dir)) * inv_emit_caustic;

            xtcore::Ray ray;
            ray.origin = lp + ln * EPSILON;
            ray.direction = dir;

            trace_photon(ray, power, 1.0, ctx->params.rdepth, false, false, true);
        }
    }

    if (!m_global_photons.empty()) {
        m_debug_global_points.reserve(m_global_photons.size());
        for (size_t i = 0; i < m_global_photons.size(); ++i) {
            m_debug_global_points.push_back(m_global_photons[i].position);
        }
        m_global_map.build(std::move(m_global_photons), PhotonPositionAccessor());
    }

    if (!m_caustic_photons.empty()) {
        m_debug_caustic_points.reserve(m_caustic_photons.size());
        for (size_t i = 0; i < m_caustic_photons.size(); ++i) {
            m_debug_caustic_points.push_back(m_caustic_photons[i].position);
        }
        m_caustic_map.build(std::move(m_caustic_photons), PhotonPositionAccessor());
    }
}

nimg::ColorRGBf Integrator::estimate_indirect(const xtcore::math::KDTree3<Photon, PhotonPositionAccessor> &map,
                                              nmath::scalar_t gather_radius,
                                              size_t gather_k,
                                              const xtcore::hit_record_t &hit,
                                              const nimg::ColorRGBf &kd) const
{
    if (map.empty() || gather_radius <= (nmath::scalar_t)EPSILON) return nimg::ColorRGBf(0, 0, 0);
    gather_k = std::max((size_t)1, gather_k);

    std::vector<photon_candidate_t> candidates;
    candidates.reserve(gather_k + 8);

    nmath::scalar_t search_radius = gather_radius;
    const nmath::scalar_t max_search_radius = std::max(search_radius, m_scene_diag * (nmath::scalar_t)2.0);

    while (true) {
        candidates.clear();
        map.radius_search(hit.point, search_radius, [&](const Photon &ph) {
            if (nmath::dot(ph.normal, hit.normal) <= (nmath::scalar_t)0.0) return;

            photon_candidate_t c;
            c.dist2 = (ph.position - hit.point).length_squared();
            c.photon = &ph;
            candidates.push_back(c);
        });

        if (candidates.size() >= gather_k || search_radius >= max_search_radius) break;
        search_radius = std::min(max_search_radius, search_radius * (nmath::scalar_t)2.0);
    }

    if (candidates.empty()) return nimg::ColorRGBf(0, 0, 0);

    size_t used = candidates.size();
    if (used > gather_k) {
        std::nth_element(
            candidates.begin(),
            candidates.begin() + static_cast<std::vector<photon_candidate_t>::difference_type>(gather_k),
            candidates.end(),
            [](const photon_candidate_t &a, const photon_candidate_t &b) {
                return a.dist2 < b.dist2;
            }
        );
        used = gather_k;
    }

    nimg::ColorRGBf flux(0, 0, 0);
    nmath::scalar_t max_r2 = 0.0;
    for (size_t i = 0; i < used; ++i) {
        flux += candidates[i].photon->power;
        max_r2 = std::max(max_r2, candidates[i].dist2);
    }

    if (!is_finite_color(flux)) return nimg::ColorRGBf(0, 0, 0);
    if (nimg::eval::luminance(flux) <= (nmath::scalar_t)EPSILON) return nimg::ColorRGBf(0, 0, 0);

        const nmath::scalar_t estimate_r2 = std::max(max_r2, (nmath::scalar_t)EPSILON);

    const nmath::scalar_t inv = (nmath::scalar_t)1.0 / (nmath::PI * nmath::PI * estimate_r2);
    return kd * flux * inv;
}

nimg::ColorRGBf Integrator::eval(size_t depth, hit_result_t &in)
{
    if (depth == 0) return nimg::ColorRGBf(0, 0, 0);

    nimg::ColorRGBf radiance(0, 0, 0);
    nimg::ColorRGBf throughput = in.intensity;
    xtcore::Ray ray = in.ray;
    nmath::scalar_t ior = in.ior;

    for (size_t bounce = 0; bounce < depth; ++bounce) {
        xtcore::hit_record_t hit;
        if (!ctx->scene.intersection(ray, hit)) {
            radiance += throughput * ctx->scene.sample_environment(ray.direction);
            break;
        }

        hit.ior = ior;
        const xtcore::asset::IMaterial *mat = ctx->scene.get_material(hit.id_object);
        if (!mat) break;

        if (mat->is_emissive()) {
            radiance += throughput * mat->get_sample(MAT_SAMPLER_EMISSIVE, hit.texcoord);
            break;
        }

        const nimg::ColorRGBf kd = mat->get_sample(MAT_SAMPLER_DIFFUSE, hit.texcoord);
        const bool diffuse_hit = nimg::eval::luminance(kd) > (nmath::scalar_t)EPSILON;

        if (diffuse_hit) {
            // Pure photon mapping: diffuse radiance comes from photon density estimation only.
            const nimg::ColorRGBf indirect_global = estimate_indirect(m_global_map, m_gather_radius, m_gather_k, hit, kd);
            const nimg::ColorRGBf indirect_caustic = estimate_indirect(m_caustic_map, m_caustic_gather_radius, m_caustic_gather_k, hit, kd);
            radiance += throughput * (indirect_global + indirect_caustic);
            break;
        }

        hit_result_t next_hit;
        next_hit.ior = ior;
        const bool cont = mat->sample_path(next_hit, hit);
        throughput *= next_hit.intensity;
        ray = next_hit.ray;
        ior = next_hit.ior;

        if (!cont) break;

        if (bounce >= 3) {
            nmath::scalar_t rr = saturate(nimg::eval::luminance(throughput));
            rr = std::max((nmath::scalar_t)0.05, std::min((nmath::scalar_t)0.95, rr));
            if (nmath::prng_c(0.0, 1.0) > rr) break;
            throughput *= (1.0 / rr);
        }
    }

    return radiance;
}

void Integrator::render_tile(xtcore::render::tile_t *tile)
{
    xtcore::asset::ICamera *cam = ctx->active_camera();

    while (tile->samples.count() > 0) {
        xtcore::antialiasing::sample_rgba_t aa_sample;
        tile->samples.pop(aa_sample);

        nimg::ColorRGBAf color_pixel;
        tile->read(aa_sample.pixel.x, aa_sample.pixel.y, color_pixel);

        hit_result_t hit_result;
        hit_result.intensity = ColorRGBf(1, 1, 1);
        hit_result.ior = 1.f;
        hit_result.ray = cam->get_primary_ray(
              aa_sample.coords.x, aa_sample.coords.y,
              (float)(ctx->params.width),
              (float)(ctx->params.height)
        );

        color_pixel += eval(ctx->params.rdepth, hit_result) * aa_sample.weight;
        color_pixel.a(1);
        tile->write(floor(aa_sample.pixel.x), floor(aa_sample.pixel.y), color_pixel);
    }
}

} /* namespace photon_mapping */
} /* namespace integrator */
} /* namespace xtcore */
