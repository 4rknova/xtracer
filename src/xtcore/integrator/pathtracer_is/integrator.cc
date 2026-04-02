#include <algorithm>
#include <map>
#include <vector>
#include <nmath/precision.h>
#include <nmath/prng.h>
#include <nimg/luminance.h>
#include <xtcore/tile.h>
#include <xtcore/aa.h>
#include <xtcore/matdefs.h>
#include <xtcore/math/sampling_util.h>
#include <xtcore/math/sphere.h>
#include <xtcore/math/triangle.h>
#include <xtcore/mesh.h>
#include <xtcore/material/boundary.h>
#include <xtcore/medium_util.h>

#include "integrator.h"

namespace xtcore {
    namespace integrator {
        namespace pathtracer_is {

namespace {

typedef Integrator::area_light_t area_light_t;

inline nmath::scalar_t triangle_area(const nmath::Vector3f &a, const nmath::Vector3f &b, const nmath::Vector3f &c)
{
    const nmath::Vector3f cr = nmath::cross(b - a, c - a);
    return (nmath::scalar_t)0.5 * cr.length();
}

inline nmath::scalar_t light_area(const xtcore::asset::ISurface *surface)
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

inline void collect_area_lights(xtcore::render::context_t *ctx, std::vector<area_light_t> &lights)
{
    lights.clear();
    if (!ctx) return;

    for (auto it = ctx->scene.m_objects.begin(); it != ctx->scene.m_objects.end(); ++it) {
        const HASH_ID obj_id = (*it).first;
        const xtcore::asset::Object *obj = (*it).second;
        if (!obj) continue;

        const xtcore::asset::ISurface *surface = ctx->scene.get_surface(obj_id);
        const xtcore::asset::IMaterial *material = ctx->scene.get_material(obj_id);
        if (!surface || !material || !material->is_emissive()) continue;

        const nmath::scalar_t area = light_area(surface);
        if (area <= (nmath::scalar_t)EPSILON) continue;

        area_light_t light;
        light.object_id = obj_id;
        light.surface = surface;
        light.material = material;
        light.area = area;
        lights.push_back(light);
    }
}

inline void sample_barycentric(nmath::scalar_t &b0, nmath::scalar_t &b1, nmath::scalar_t &b2)
{
    const nmath::scalar_t u = nmath::prng_c(0.0, 1.0);
    const nmath::scalar_t v = nmath::prng_c(0.0, 1.0);
    const nmath::scalar_t su = nmath_sqrt(std::max((nmath::scalar_t)0.0, u));

    b0 = (nmath::scalar_t)1.0 - su;
    b1 = su * ((nmath::scalar_t)1.0 - v);
    b2 = su * v;
}

inline bool sample_light_point(const area_light_t &light, nmath::Vector3f &point, nmath::Vector3f &normal, nmath::Vector3f &texcoord, nmath::scalar_t &pdf_area)
{
    if (!light.surface || light.area <= (nmath::scalar_t)EPSILON) return false;

    const xtcore::surface::Sphere *sp = dynamic_cast<const xtcore::surface::Sphere *>(light.surface);
    if (sp) {
        point = sp->point_sample();
        normal = (point - sp->origin).normalized();
        const nmath::scalar_t uvsx = (sp->uv_scale.x != 0.0f ? sp->uv_scale.x : 1.0f);
        const nmath::scalar_t uvsy = (sp->uv_scale.y != 0.0f ? sp->uv_scale.y : 1.0f);
        texcoord = nmath::Vector3f(
            (nmath_asin(normal.x / uvsx) / nmath::PI + 0.5),
            (nmath_asin(normal.y / uvsy) / nmath::PI + 0.5),
            0.0
        );
        pdf_area = 1.0 / light.area;
        return true;
    }

    const xtcore::surface::Triangle *tr = dynamic_cast<const xtcore::surface::Triangle *>(light.surface);
    if (tr) {
        nmath::scalar_t b0, b1, b2;
        sample_barycentric(b0, b1, b2);
        point = tr->v[0] * b0 + tr->v[1] * b1 + tr->v[2] * b2;
        normal = tr->n[0] * b0 + tr->n[1] * b1 + tr->n[2] * b2;
        if (normal.length() <= (nmath::scalar_t)EPSILON) normal = tr->calc_normal();
        normal.normalize();
        nmath::Vector2f uv = tr->tc[0] * b0 + tr->tc[1] * b1 + tr->tc[2] * b2;
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
        sample_barycentric(b0, b1, b2);
        point = selected->v[0] * b0 + selected->v[1] * b1 + selected->v[2] * b2;
        normal = selected->n[0] * b0 + selected->n[1] * b1 + selected->n[2] * b2;
        if (normal.length() <= (nmath::scalar_t)EPSILON) normal = selected->calc_normal();
        normal.normalize();
        nmath::Vector2f uv = selected->tc[0] * b0 + selected->tc[1] * b1 + selected->tc[2] * b2;
        const nmath::scalar_t uvsx = (mesh->uv_scale.x != 0.0f ? mesh->uv_scale.x : 1.0f);
        const nmath::scalar_t uvsy = (mesh->uv_scale.y != 0.0f ? mesh->uv_scale.y : 1.0f);
        texcoord = nmath::Vector3f(uv.x * uvsx, uv.y * uvsy, 0.0);
        pdf_area = 1.0 / light.area;
        return true;
    }

    return false;
}

inline nmath::scalar_t power_heuristic(nmath::scalar_t p_a, nmath::scalar_t p_b)
{
    const nmath::scalar_t a2 = p_a * p_a;
    const nmath::scalar_t b2 = p_b * p_b;
    const nmath::scalar_t denom = a2 + b2;
    if (denom <= (nmath::scalar_t)EPSILON) return 0.0;
    return a2 / denom;
}

inline bool visible_to_light(xtcore::render::context_t *ctx, const nmath::Vector3f &origin, const nmath::Vector3f &target, HASH_ID light_object_id)
{
    const nmath::Vector3f v = target - origin;
    const nmath::scalar_t dist = v.length();
    if (dist <= (nmath::scalar_t)EPSILON) return false;

    const nmath::Vector3f dir = v / dist;
    xtcore::Ray shadow_ray;
    shadow_ray.origin = origin + dir * EPSILON;
    shadow_ray.direction = dir;

    for (size_t step = 0; step < 16; ++step) {
        xtcore::hit_record_t occ;
        if (!ctx->scene.intersection(shadow_ray, occ)) return false;

        if (occ.t >= dist - (nmath::scalar_t)1e-4) return occ.id_object == light_object_id;
        if (occ.id_object == light_object_id) return true;

        const xtcore::asset::IMaterial *mat = ctx->scene.get_material(occ.id_object);
        if (!mat) return false;
        if (dynamic_cast<const xtcore::asset::material::Boundary *>(mat) == nullptr) return false;

        shadow_ray.origin = occ.point + dir * EPSILON;
    }

    return false;
}

inline nmath::scalar_t clamp_scalar(nmath::scalar_t v, nmath::scalar_t lo, nmath::scalar_t hi)
{
    return std::max(lo, std::min(v, hi));
}

} // namespace

void Integrator::setup_auxiliary()
{
    m_lights.clear();
    m_light_index_by_objid.clear();
    collect_area_lights(ctx, m_lights);
    for (size_t i = 0; i < m_lights.size(); ++i) {
        m_light_index_by_objid[m_lights[i].object_id] = i;
    }
}

void Integrator::clean_auxiliary()
{
    m_lights.clear();
    m_light_index_by_objid.clear();
}

nimg::ColorRGBf Integrator::eval(size_t depth, hit_result_t &in)
{
    if (depth == 0) return nimg::ColorRGBf(0, 0, 0);

    nimg::ColorRGBf radiance(0, 0, 0);
    nimg::ColorRGBf throughput = in.intensity;
    xtcore::Ray ray = in.ray;
    nmath::scalar_t ior = in.ior;
    bool prev_was_diffuse_sample = false;
    nmath::scalar_t prev_bsdf_pdf = 0.0;
    nmath::Vector3f prev_point;
    HASH_ID current_medium_object_id = HASH_ID_INVALID;
    nmath::scalar_t current_medium_exit = INFINITY;
    const xtcore::asset::medium::IMedium *current_medium =
        xtcore::medium::find_containing_medium(ctx->scene, ray.origin, current_medium_object_id, current_medium_exit, ray.direction);

    for (size_t bounce = 0; bounce < depth; ++bounce) {
        xtcore::hit_record_t hit_record;
        bool hit = ctx->scene.intersection(ray, hit_record);
        const nmath::scalar_t t_surface = hit ? hit_record.t : INFINITY;

        nmath::scalar_t t_exit = INFINITY;
        const xtcore::asset::medium::IMedium *medium = current_medium;
        if (medium) {
            if (!xtcore::medium::distance_to_medium_boundary(ctx->scene, current_medium_object_id, ray.origin, ray.direction, t_exit)) {
                current_medium_object_id = HASH_ID_INVALID;
                current_medium_exit = INFINITY;
                current_medium = 0;
                medium = 0;
            } else {
                current_medium_exit = t_exit;
            }
        }
        if (medium) {
            const nmath::scalar_t segment_dist = std::min(t_surface, t_exit);
            const bool exits_before_surface = t_exit <= t_surface + (nmath::scalar_t)EPSILON;

            nmath::scalar_t event_dist = segment_dist;
            const bool scatter = xtcore::medium::sample_distance(*medium, ray.origin, ray.direction, segment_dist, event_dist);
            const nimg::ColorRGBf tr = xtcore::medium::transmittance(*medium, ray.origin, ray.direction, event_dist);
            const nmath::Vector3f event_pos = ray.origin + ray.direction * event_dist;
            radiance += throughput * xtcore::medium::emission(*medium, event_pos) * event_dist;
            throughput *= tr;

            if (scatter) {
                throughput *= xtcore::medium::scattering_weight(*medium, event_pos);

                if (!m_lights.empty()) {
                    const nmath::scalar_t p_select = 1.0 / (nmath::scalar_t)m_lights.size();
                    size_t light_idx = (size_t)(nmath::prng_c(0.0, 1.0) * (nmath::scalar_t)m_lights.size());
                    if (light_idx >= m_lights.size()) light_idx = m_lights.size() - 1;

                    const area_light_t &light = m_lights[light_idx];
                    nmath::Vector3f lp, ln, ltc;
                    nmath::scalar_t p_area = 0.0;
                    if (sample_light_point(light, lp, ln, ltc, p_area)) {
                        const nmath::Vector3f scatter_pos = event_pos;
                        const nmath::Vector3f to_light = lp - scatter_pos;
                        const nmath::scalar_t dist2 = to_light.length_squared();
                        if (dist2 > (nmath::scalar_t)EPSILON) {
                            const nmath::Vector3f wi = to_light / nmath_sqrt(dist2);
                            const nmath::scalar_t cos_l = std::max((nmath::scalar_t)0.0, nmath::dot(ln, -wi));
                            if (cos_l > (nmath::scalar_t)EPSILON &&
                                visible_to_light(ctx, scatter_pos + wi * EPSILON, lp, light.object_id)) {
                                const nmath::scalar_t p_light = p_select * p_area * dist2 / cos_l;
                                if (p_light > (nmath::scalar_t)EPSILON) {
                                    const nmath::scalar_t phase = xtcore::medium::phase_hg(nmath::dot(-ray.direction, wi), medium->asymmetry());
                                    const nimg::ColorRGBf le = light.material->get_sample(MAT_SAMPLER_EMISSIVE, ltc);
                                    const nmath::scalar_t tr_dist = std::min(nmath_sqrt(dist2), std::max((nmath::scalar_t)0.0, t_exit - event_dist));
                                    const nimg::ColorRGBf tr_light = xtcore::medium::transmittance(*medium, scatter_pos, wi, tr_dist);
                                    radiance += throughput * tr_light * le * (phase / p_light);
                                }
                            }
                        }
                    }
                }

                const nmath::Vector3f scatter_pos = event_pos;
                const nmath::Vector3f new_dir = xtcore::medium::sample_hg_direction(ray.direction, medium->asymmetry());
                ray.origin = scatter_pos + new_dir * EPSILON;
                ray.direction = new_dir;
                current_medium_exit = std::max((nmath::scalar_t)0.0, t_exit - event_dist);
                prev_was_diffuse_sample = false;
                prev_bsdf_pdf = 0.0;
                prev_point = scatter_pos;
                continue;
            }

            if (exits_before_surface) {
                ray.origin = ray.origin + ray.direction * (segment_dist + (nmath::scalar_t)EPSILON);
                current_medium_object_id = HASH_ID_INVALID;
                current_medium_exit = INFINITY;
                current_medium = 0;
                prev_was_diffuse_sample = false;
                prev_bsdf_pdf = 0.0;
                continue;
            }
        }

        if (!hit) {
            radiance += throughput * ctx->scene.sample_environment(ray.direction);
            break;
        }

        hit_record.ior = ior;
        const xtcore::asset::IMaterial *m = ctx->scene.get_material(hit_record.id_object);
        if (!m) break;
        const xtcore::asset::medium::IMedium *boundary_medium = ctx->scene.get_object_medium(hit_record.id_object);
        const xtcore::asset::material::Boundary *boundary = dynamic_cast<const xtcore::asset::material::Boundary *>(m);

        if (m->is_emissive()) {
            nimg::ColorRGBf le = m->get_sample(MAT_SAMPLER_EMISSIVE, hit_record.texcoord);
            nmath::scalar_t mis_w = 1.0;

            if (bounce > 0 && prev_was_diffuse_sample && !m_lights.empty()) {
                auto lit = m_light_index_by_objid.find(hit_record.id_object);
                if (lit != m_light_index_by_objid.end()) {
                    const area_light_t &light = m_lights[(*lit).second];
                    const nmath::scalar_t cos_light = std::max((nmath::scalar_t)0.0, nmath::dot(hit_record.normal, -ray.direction));
                    const nmath::Vector3f d = hit_record.point - prev_point;
                    const nmath::scalar_t dist2 = d.length_squared();
                    if (cos_light > (nmath::scalar_t)EPSILON && dist2 > (nmath::scalar_t)EPSILON) {
                        const nmath::scalar_t p_select = 1.0 / (nmath::scalar_t)m_lights.size();
                        const nmath::scalar_t p_area = 1.0 / light.area;
                        const nmath::scalar_t p_light = p_select * p_area * dist2 / cos_light;
                        mis_w = power_heuristic(prev_bsdf_pdf, p_light);
                    }
                }
            }

            radiance += throughput * le * mis_w;
            break;
        }

        const nimg::ColorRGBf kd = m->get_sample(MAT_SAMPLER_DIFFUSE, hit_record.texcoord);
        const nmath::scalar_t kd_luma = nimg::eval::luminance(kd);

        if (kd_luma > (nmath::scalar_t)0.0) {
            if (!m_lights.empty()) {
                const nmath::scalar_t p_select = 1.0 / (nmath::scalar_t)m_lights.size();
                size_t light_idx = (size_t)(nmath::prng_c(0.0, 1.0) * (nmath::scalar_t)m_lights.size());
                if (light_idx >= m_lights.size()) light_idx = m_lights.size() - 1;
                const area_light_t &light = m_lights[light_idx];

                nmath::Vector3f lp, ln, ltc;
                nmath::scalar_t p_area = 0.0;
                if (sample_light_point(light, lp, ln, ltc, p_area)) {
                    const nmath::Vector3f to_light = lp - hit_record.point;
                    const nmath::scalar_t dist2 = to_light.length_squared();
                    if (dist2 > (nmath::scalar_t)EPSILON) {
                        const nmath::Vector3f wi = to_light / nmath_sqrt(dist2);
                        const nmath::scalar_t cos_s = std::max((nmath::scalar_t)0.0, nmath::dot(hit_record.normal, wi));
                        const nmath::scalar_t cos_l = std::max((nmath::scalar_t)0.0, nmath::dot(ln, -wi));

                        if (cos_s > (nmath::scalar_t)EPSILON &&
                            cos_l > (nmath::scalar_t)EPSILON &&
                            visible_to_light(ctx, hit_record.point + hit_record.normal * EPSILON, lp, light.object_id)) {
                            const nimg::ColorRGBf le = light.material->get_sample(MAT_SAMPLER_EMISSIVE, ltc);
                            const nimg::ColorRGBf f = kd * (1.0 / nmath::PI);
                            const nmath::scalar_t p_light = p_select * p_area * dist2 / cos_l;
                            if (p_light > (nmath::scalar_t)EPSILON) {
                                const nmath::scalar_t p_bsdf = cos_s / nmath::PI;
                                const nmath::scalar_t mis_w = power_heuristic(p_light, p_bsdf);
                                radiance += throughput * f * le * (cos_s / p_light) * mis_w;
                            }
                        }
                    }
                }
            }

            nmath::scalar_t pdf = 0.0;
            nmath::Vector3f wo = xtcore::math::sampling::sample_cosine_hemisphere(hit_record.normal, pdf);
            const nmath::scalar_t cos_theta = std::max((nmath::scalar_t)0.0, nmath::dot(hit_record.normal, wo));

            if (pdf <= (nmath::scalar_t)EPSILON || cos_theta <= (nmath::scalar_t)0.0) break;

            nimg::ColorRGBf bsdf = kd * (1.0 / nmath::PI);
            throughput *= bsdf * (cos_theta / pdf);

            prev_point = hit_record.point;
            prev_bsdf_pdf = pdf;
            prev_was_diffuse_sample = true;

            ray.origin = hit_record.point + hit_record.normal * EPSILON;
            ray.direction = wo;
        } else {
            xtcore::hit_result_t next_hit;
            next_hit.ior = ior;
            bool path_continues = m->sample_path(next_hit, hit_record);
            throughput *= next_hit.intensity;
            ray = next_hit.ray;
            ior = next_hit.ior;

            if (boundary && boundary_medium) {
                const nmath::scalar_t medium_side = nmath::dot(hit_record.incident_direction.normalized(), hit_record.normal.normalized());
                if (medium_side < (nmath::scalar_t)0.0) {
                    current_medium_object_id = hit_record.id_object;
                    current_medium = boundary_medium;
                    if (!xtcore::medium::distance_to_medium_boundary(ctx->scene, current_medium_object_id, ray.origin, ray.direction, current_medium_exit)) {
                        current_medium_object_id = HASH_ID_INVALID;
                        current_medium_exit = INFINITY;
                        current_medium = 0;
                    }
                } else if (current_medium_object_id == hit_record.id_object) {
                    current_medium_object_id = HASH_ID_INVALID;
                    current_medium_exit = INFINITY;
                    current_medium = 0;
                }
            }

            prev_was_diffuse_sample = false;
            prev_bsdf_pdf = 0.0;
            if (!path_continues) break;
        }

        if (bounce >= 3) {
            const nmath::scalar_t rr = clamp_scalar((nmath::scalar_t)nimg::eval::luminance(throughput), (nmath::scalar_t)0.1, (nmath::scalar_t)0.95);
            if (nmath::prng_c(0.0, 1.0) > rr) break;
            throughput *= (1.0 / rr);
        }
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

        } /* namespace pathtracer_is */
    } /* namespace integrator */
} /* namespace xtcore */
