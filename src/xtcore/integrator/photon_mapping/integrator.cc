#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <map>
#include <vector>

#include <nmath/precision.h>
#include <nmath/prng.h>
#include <nimg/luminance.h>
#include <xtcore/aa.h>
#include <xtcore/matdefs.h>
#include <xtcore/math/sphere.h>
#include <xtcore/math/triangle.h>
#include <xtcore/mesh.h>
#include <xtcore/tile.h>

#include "integrator.h"

namespace xtcore {
    namespace integrator {
        namespace photon_mapping {

namespace {

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
        nmath::scalar_t area = 0.0;
        const std::vector<xtcore::surface::Triangle> &tris = mesh->triangles();
        for (size_t i = 0; i < tris.size(); ++i) {
            area += triangle_area(tris[i].v[0], tris[i].v[1], tris[i].v[2]);
        }
        return area;
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

inline nmath::Vector3f build_tangent(const nmath::Vector3f &n)
{
    const nmath::Vector3f up = (nmath_abs(n.z) < 0.999)
                             ? nmath::Vector3f(0.0, 0.0, 1.0)
                             : nmath::Vector3f(1.0, 0.0, 0.0);
    nmath::Vector3f t = nmath::cross(up, n);
    t.normalize();
    return t;
}

inline nmath::Vector3f sample_cosine_hemisphere(const nmath::Vector3f &normal, nmath::scalar_t &out_pdf)
{
    const nmath::scalar_t u1 = nmath::prng_c(0.0, 1.0);
    const nmath::scalar_t u2 = nmath::prng_c(0.0, 1.0);
    const nmath::scalar_t r = nmath_sqrt(u1);
    const nmath::scalar_t theta = nmath::PI_DOUBLE * u2;

    const nmath::scalar_t x = r * nmath_cos(theta);
    const nmath::scalar_t z = r * nmath_sin(theta);
    const nmath::scalar_t y = nmath_sqrt(std::max((nmath::scalar_t)0.0, (nmath::scalar_t)(1.0 - u1)));

    const nmath::Vector3f n = normal.normalized();
    const nmath::Vector3f t = build_tangent(n);
    const nmath::Vector3f b = nmath::cross(n, t).normalized();

    nmath::Vector3f dir = (t * x) + (n * y) + (b * z);
    dir.normalize();

    const nmath::scalar_t cos_theta = std::max((nmath::scalar_t)0.0, nmath::dot(n, dir));
    out_pdf = cos_theta / nmath::PI;
    return dir;
}

inline bool sample_light_point(
    const Integrator::AreaLight &light,
    nmath::Vector3f &point,
    nmath::Vector3f &normal,
    nmath::Vector3f &texcoord,
    nmath::scalar_t &pdf_area
)
{
    if (!light.surface || light.area <= (nmath::scalar_t)EPSILON) return false;

    const xtcore::surface::Sphere *sp = dynamic_cast<const xtcore::surface::Sphere *>(light.surface);
    if (sp) {
        point = sp->point_sample();
        normal = (point - sp->origin).normalized();
        const nmath::scalar_t sx = (sp->uv_scale.x != 0.0f ? sp->uv_scale.x : 1.0f);
        const nmath::scalar_t sy = (sp->uv_scale.y != 0.0f ? sp->uv_scale.y : 1.0f);
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
        texcoord = nmath::Vector3f(uv.x, uv.y, 0.0);
        pdf_area = 1.0 / light.area;
        return true;
    }

    return false;
}

inline bool is_finite_scalar(nmath::scalar_t v)
{
    return std::isfinite((double)v) != 0;
}

} // namespace

Integrator::Integrator()
    : m_root(-1)
    , m_scene_diag(10.0)
    , m_gather_radius(0.25)
    , m_gather_k(64)
    , m_emit_photons(20000)
    , m_override_gather_radius(false)
    , m_override_gather_k(false)
    , m_override_emit_photons(false)
    , m_config_gather_radius(0.25)
    , m_config_gather_k(64)
    , m_config_emit_photons(20000)
{}

void Integrator::configure(const std::map<std::string, std::string> &options)
{
    auto it_emit = options.find("emit_photons");
    m_override_emit_photons = false;
    if (it_emit != options.end()) {
        char *end = nullptr;
        unsigned long long v = std::strtoull(it_emit->second.c_str(), &end, 10);
        if (!(end == it_emit->second.c_str() || !end || *end != '\0') && v > 0) {
            m_config_emit_photons = (size_t)v;
            m_override_emit_photons = true;
        }
    }

    auto it_radius = options.find("gather_radius");
    m_override_gather_radius = false;
    if (it_radius != options.end()) {
        char *end = nullptr;
        double v = std::strtod(it_radius->second.c_str(), &end);
        if (!(end == it_radius->second.c_str() || !end || *end != '\0') && v > 0.0) {
            m_config_gather_radius = (nmath::scalar_t)v;
            m_override_gather_radius = true;
        }
    }

    auto it_k = options.find("gather_k");
    m_override_gather_k = false;
    if (it_k != options.end()) {
        char *end = nullptr;
        unsigned long long v = std::strtoull(it_k->second.c_str(), &end, 10);
        if (!(end == it_k->second.c_str() || !end || *end != '\0') && v > 0) {
            m_config_gather_k = (size_t)v;
            m_override_gather_k = true;
        }
    }
}

void Integrator::setup_auxiliary()
{
    build_photon_map();
}

void Integrator::clean_auxiliary()
{
    m_photons.clear();
    m_nodes.clear();
    m_lights.clear();
    m_root = -1;
}

int Integrator::build_kdtree(size_t begin, size_t end, int axis)
{
    if (begin >= end) return -1;

    const size_t mid = begin + (end - begin) / 2;
    std::nth_element(
        m_photons.begin() + begin,
        m_photons.begin() + mid,
        m_photons.begin() + end,
        [axis](const Photon &a, const Photon &b) {
            return a.position[axis] < b.position[axis];
        }
    );

    PhotonNode node;
    node.photon = m_photons[mid];
    node.axis = axis;
    node.left = -1;
    node.right = -1;

    const int idx = (int)m_nodes.size();
    m_nodes.push_back(node);

    const int next_axis = (axis + 1) % 3;
    m_nodes[idx].left = build_kdtree(begin, mid, next_axis);
    m_nodes[idx].right = build_kdtree(mid + 1, end, next_axis);
    return idx;
}

void Integrator::trace_photon(const xtcore::Ray &in_ray, const nimg::ColorRGBf &in_power, nmath::scalar_t in_ior, size_t depth)
{
    if (depth == 0) return;

    xtcore::Ray ray = in_ray;
    nimg::ColorRGBf power = in_power;
    nmath::scalar_t ior = in_ior;

    for (size_t bounce = 0; bounce < depth; ++bounce) {
        xtcore::hit_record_t hit;
        if (!ctx->scene.intersection(ray, hit)) return;
        hit.ior = ior;

        const xtcore::asset::IMaterial *mat = ctx->scene.get_material(hit.id_object);
        if (!mat) return;
        if (mat->is_emissive()) return;

        const nimg::ColorRGBf kd = mat->get_sample(MAT_SAMPLER_DIFFUSE, hit.texcoord);
        const nmath::scalar_t kd_luma = nimg::eval::luminance(kd);

        if (kd_luma > (nmath::scalar_t)0.0) {
            Photon ph;
            ph.position = hit.point;
            ph.normal = hit.normal.normalized();
            ph.incident = -ray.direction.normalized();
            ph.power = power;
            m_photons.push_back(ph);

            nmath::scalar_t pdf = 0.0;
            nmath::Vector3f wo = sample_cosine_hemisphere(hit.normal, pdf);
            if (pdf <= (nmath::scalar_t)EPSILON) return;

            power *= kd;
            ray.origin = hit.point + hit.normal * EPSILON;
            ray.direction = wo;
        } else {
            xtcore::hit_result_t next_hit;
            next_hit.ior = ior;
            const bool cont = mat->sample_path(next_hit, hit);
            power *= next_hit.intensity;
            ray = next_hit.ray;
            ior = next_hit.ior;
            if (!cont) return;
        }

        if (bounce >= 3) {
            nmath::scalar_t rr = nimg::eval::luminance(power);
            rr = std::max((nmath::scalar_t)0.1, std::min((nmath::scalar_t)0.95, rr));
            if (nmath::prng_c(0.0, 1.0) > rr) return;
            power *= (1.0 / rr);
        }
    }
}

void Integrator::build_photon_map()
{
    m_photons.clear();
    m_nodes.clear();
    m_lights.clear();
    m_root = -1;

    nmath::Vector3f bmin( std::numeric_limits<nmath::scalar_t>::max(),
                          std::numeric_limits<nmath::scalar_t>::max(),
                          std::numeric_limits<nmath::scalar_t>::max());
    nmath::Vector3f bmax(-std::numeric_limits<nmath::scalar_t>::max(),
                         -std::numeric_limits<nmath::scalar_t>::max(),
                         -std::numeric_limits<nmath::scalar_t>::max());
    bool has_bounds = false;

    nmath::scalar_t cdf = 0.0;
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

        if (!material->is_emissive()) continue;

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
    m_gather_k = m_override_gather_k ? std::max((size_t)1, m_config_gather_k) : (size_t)64;

    if (m_lights.empty()) return;

    m_emit_photons = std::max((size_t)20000, ctx->params.samples * (size_t)8000);
    m_emit_photons = std::min((size_t)120000, m_emit_photons);
    if (m_override_emit_photons) m_emit_photons = std::max((size_t)1, m_config_emit_photons);
    m_photons.reserve(m_emit_photons);

    const nmath::scalar_t total_cdf = m_lights.back().weight_cdf;

    for (size_t i = 0; i < m_emit_photons; ++i) {
        const nmath::scalar_t u = nmath::prng_c(0.0, 1.0) * total_cdf;
        size_t light_idx = 0;
        while (light_idx + 1 < m_lights.size() && m_lights[light_idx].weight_cdf < u) ++light_idx;
        const AreaLight &light = m_lights[light_idx];

        const nmath::scalar_t prev_cdf = (light_idx == 0 ? (nmath::scalar_t)0.0 : m_lights[light_idx - 1].weight_cdf);
        const nmath::scalar_t light_prob = (light.weight_cdf - prev_cdf) / total_cdf;
        if (light_prob <= (nmath::scalar_t)EPSILON) continue;

        nmath::Vector3f lp, ln, ltc;
        nmath::scalar_t p_area = 0.0;
        if (!sample_light_point(light, lp, ln, ltc, p_area)) continue;
        if (p_area <= (nmath::scalar_t)EPSILON) continue;

        nmath::scalar_t p_dir = 0.0;
        nmath::Vector3f dir = sample_cosine_hemisphere(ln, p_dir);
        if (p_dir <= (nmath::scalar_t)EPSILON) continue;

        const nimg::ColorRGBf le = light.material->get_sample(MAT_SAMPLER_EMISSIVE, ltc);
        const nimg::ColorRGBf power = le * (nmath::PI / (light_prob * p_area * (nmath::scalar_t)m_emit_photons));

        xtcore::Ray pr;
        pr.origin = lp + ln * EPSILON;
        pr.direction = dir;

        trace_photon(pr, power, 1.0, ctx->params.rdepth);
    }

    if (m_photons.empty()) return;

    m_nodes.reserve(m_photons.size());
    m_root = build_kdtree(0, m_photons.size(), 0);
}

nimg::ColorRGBf Integrator::estimate_indirect(const xtcore::hit_record_t &hit, const nimg::ColorRGBf &kd) const
{
    if (m_root < 0 || m_nodes.empty()) return nimg::ColorRGBf(0, 0, 0);

    const nmath::scalar_t r2 = m_gather_radius * m_gather_radius;
    nimg::ColorRGBf flux(0, 0, 0);
    size_t count = 0;

    struct stack_item_t { int node; };
    std::vector<stack_item_t> stack;
    stack.push_back({m_root});

    while (!stack.empty()) {
        const int node_idx = stack.back().node;
        stack.pop_back();
        if (node_idx < 0) continue;

        const PhotonNode &node = m_nodes[(size_t)node_idx];
        const Photon &ph = node.photon;

        const nmath::Vector3f d = ph.position - hit.point;
        const nmath::scalar_t d2 = d.length_squared();
        if (d2 <= r2 && nmath::dot(ph.normal, hit.normal) > (nmath::scalar_t)0.0) {
            flux += ph.power;
            ++count;
        }

        const nmath::scalar_t delta = hit.point[node.axis] - ph.position[node.axis];
        const int near_node = delta < (nmath::scalar_t)0.0 ? node.left : node.right;
        const int far_node = delta < (nmath::scalar_t)0.0 ? node.right : node.left;

        if (near_node >= 0) stack.push_back({near_node});
        if (far_node >= 0 && delta * delta <= r2) stack.push_back({far_node});
    }

    if (count == 0) return nimg::ColorRGBf(0, 0, 0);

    // Jensen radiance estimate for Lambertian surfaces: L ~= (kd / pi) * (sum Phi / (pi r^2)).
    return kd * ((nmath::scalar_t)1.0 / (nmath::PI * nmath::PI * r2)) * flux;
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
        const nmath::scalar_t kd_luma = nimg::eval::luminance(kd);

        if (kd_luma > (nmath::scalar_t)0.0) {
            // Jensen two-pass style: direct (from light sampling) + indirect (from photon map density estimate).
            nimg::ColorRGBf direct(0, 0, 0);

            if (!m_lights.empty()) {
                size_t li = (size_t)(nmath::prng_c(0.0, 1.0) * (nmath::scalar_t)m_lights.size());
                if (li >= m_lights.size()) li = m_lights.size() - 1;
                const AreaLight &light = m_lights[li];

                nmath::Vector3f lp, ln, ltc;
                nmath::scalar_t p_area = 0.0;
                if (sample_light_point(light, lp, ln, ltc, p_area) && p_area > (nmath::scalar_t)EPSILON) {
                    const nmath::Vector3f to_light = lp - hit.point;
                    const nmath::scalar_t dist2 = to_light.length_squared();
                    if (dist2 > (nmath::scalar_t)EPSILON) {
                        const nmath::scalar_t dist = nmath_sqrt(dist2);
                        const nmath::Vector3f wi = to_light / dist;
                        const nmath::scalar_t cos_s = std::max((nmath::scalar_t)0.0, nmath::dot(hit.normal, wi));
                        const nmath::scalar_t cos_l = std::max((nmath::scalar_t)0.0, nmath::dot(ln, -wi));
                        if (cos_s > (nmath::scalar_t)EPSILON && cos_l > (nmath::scalar_t)EPSILON) {
                            xtcore::Ray shadow;
                            shadow.origin = hit.point + hit.normal * EPSILON;
                            shadow.direction = wi;

                            xtcore::hit_record_t occ;
                            if (ctx->scene.intersection(shadow, occ) &&
                                occ.id_object == light.object_id &&
                                occ.t >= dist - (nmath::scalar_t)1e-4) {
                                const nimg::ColorRGBf le = light.material->get_sample(MAT_SAMPLER_EMISSIVE, ltc);
                                const nmath::scalar_t p_select = 1.0 / (nmath::scalar_t)m_lights.size();
                                const nmath::scalar_t p_light = p_select * p_area * dist2 / cos_l;
                                if (p_light > (nmath::scalar_t)EPSILON) {
                                    const nimg::ColorRGBf f = kd * (1.0 / nmath::PI);
                                    direct += f * le * (cos_s / p_light);
                                }
                            }
                        }
                    }
                }
            }

            const nimg::ColorRGBf indirect = estimate_indirect(hit, kd);
            radiance += throughput * (direct + indirect);
            break;
        }

        xtcore::hit_result_t next_hit;
        next_hit.ior = ior;
        const bool cont = mat->sample_path(next_hit, hit);
        throughput *= next_hit.intensity;
        ray = next_hit.ray;
        ior = next_hit.ior;
        if (!cont) break;
    }

    return radiance;
}

void Integrator::render_tile(xtcore::render::tile_t *tile)
{
    xtcore::asset::ICamera *cam = ctx->scene.get_camera(ctx->params.camera);

    while (tile->samples.count() > 0) {
        xtcore::antialiasing::sample_rgba_t aa_sample;
        tile->samples.pop(aa_sample);

        nimg::ColorRGBAf color_pixel;
        tile->read(aa_sample.pixel.x, aa_sample.pixel.y, color_pixel);

        hit_result_t hit_result;
        hit_result.intensity = ColorRGBf(1,1,1);
        hit_result.ior = 1.f;
        hit_result.ray = cam->get_primary_ray(
              aa_sample.coords.x, aa_sample.coords.y
            , (float)(ctx->params.width)
            , (float)(ctx->params.height)
        );

        color_pixel += eval(ctx->params.rdepth, hit_result) * aa_sample.weight;
        color_pixel.a(1);
        tile->write(floor(aa_sample.pixel.x), floor(aa_sample.pixel.y), color_pixel);
    }
}

        } /* namespace photon_mapping */
    } /* namespace integrator */
} /* namespace xtcore */
