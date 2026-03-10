#include <algorithm>
#include <map>
#include <vector>
#include <nmath/precision.h>
#include <nmath/prng.h>
#include <nimg/luminance.h>
#include <xtcore/tile.h>
#include <xtcore/aa.h>
#include <xtcore/matdefs.h>
#include <xtcore/math/sphere.h>
#include <xtcore/math/triangle.h>
#include <xtcore/mesh.h>

#include "integrator.h"

namespace xtcore {
    namespace integrator {
        namespace pathtracer_is {

namespace {

struct area_light_t
{
    HASH_ID object_id;
    const xtcore::asset::ISurface *surface;
    const xtcore::asset::IMaterial *material;
    nmath::scalar_t area;
};

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

        auto sit = ctx->scene.m_surface.find(obj->surface);
        auto mit = ctx->scene.m_materials.find(obj->material);
        if (sit == ctx->scene.m_surface.end() || mit == ctx->scene.m_materials.end()) continue;

        const xtcore::asset::ISurface *surface = (*sit).second;
        const xtcore::asset::IMaterial *material = (*mit).second;
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
    b0 = nmath::prng_c(0.0, 1.0);
    b1 = nmath::prng_c(0.0, 1.0);
    b2 = nmath::prng_c(0.0, 1.0);
    const nmath::scalar_t sum = b0 + b1 + b2;
    if (sum > (nmath::scalar_t)EPSILON) {
        b0 /= sum;
        b1 /= sum;
        b2 /= sum;
    }
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
        texcoord = nmath::Vector3f(uv.x, uv.y, 0.0);
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

    xtcore::Ray shadow_ray;
    shadow_ray.origin = origin + v.normalized() * EPSILON;
    shadow_ray.direction = v / dist;

    xtcore::hit_record_t occ;
    if (!ctx->scene.intersection(shadow_ray, occ)) return false;
    if (occ.id_object != light_object_id) return false;

    return occ.t >= dist - (nmath::scalar_t)1e-4;
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

    nmath::Vector3f n = normal.normalized();
    nmath::Vector3f t = build_tangent(n);
    nmath::Vector3f b = nmath::cross(n, t).normalized();

    nmath::Vector3f dir = (t * x) + (n * y) + (b * z);
    dir.normalize();

    const nmath::scalar_t cos_theta = std::max((nmath::scalar_t)0.0, nmath::dot(n, dir));
    out_pdf = cos_theta / nmath::PI;
    return dir;
}

inline nmath::scalar_t clamp_scalar(nmath::scalar_t v, nmath::scalar_t lo, nmath::scalar_t hi)
{
    return std::max(lo, std::min(v, hi));
}

} // namespace

nimg::ColorRGBf Integrator::eval(size_t depth, hit_result_t &in)
{
    if (depth == 0) return nimg::ColorRGBf(0, 0, 0);

    std::vector<area_light_t> lights;
    collect_area_lights(ctx, lights);
    std::map<HASH_ID, size_t> light_index_by_objid;
    for (size_t i = 0; i < lights.size(); ++i) {
        light_index_by_objid[lights[i].object_id] = i;
    }

    nimg::ColorRGBf radiance(0, 0, 0);
    nimg::ColorRGBf throughput = in.intensity;
    xtcore::Ray ray = in.ray;
    nmath::scalar_t ior = in.ior;
    bool prev_was_diffuse_sample = false;
    nmath::scalar_t prev_bsdf_pdf = 0.0;
    nmath::Vector3f prev_point;

    for (size_t bounce = 0; bounce < depth; ++bounce) {
        xtcore::hit_record_t hit_record;
        bool hit = ctx->scene.intersection(ray, hit_record);
        if (!hit) {
            radiance += throughput * ctx->scene.sample_environment(ray.direction);
            break;
        }

        hit_record.ior = ior;
        const xtcore::asset::IMaterial *m = ctx->scene.get_material(hit_record.id_object);
        if (!m) break;

        if (m->is_emissive()) {
            nimg::ColorRGBf le = m->get_sample(MAT_SAMPLER_EMISSIVE, hit_record.texcoord);
            nmath::scalar_t mis_w = 1.0;

            if (bounce > 0 && prev_was_diffuse_sample && !lights.empty()) {
                auto lit = light_index_by_objid.find(hit_record.id_object);
                if (lit != light_index_by_objid.end()) {
                    const area_light_t &light = lights[(*lit).second];
                    const nmath::scalar_t cos_light = std::max((nmath::scalar_t)0.0, nmath::dot(hit_record.normal, -ray.direction));
                    const nmath::Vector3f d = hit_record.point - prev_point;
                    const nmath::scalar_t dist2 = d.length_squared();
                    if (cos_light > (nmath::scalar_t)EPSILON && dist2 > (nmath::scalar_t)EPSILON) {
                        const nmath::scalar_t p_select = 1.0 / (nmath::scalar_t)lights.size();
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
            if (!lights.empty()) {
                const nmath::scalar_t p_select = 1.0 / (nmath::scalar_t)lights.size();
                size_t light_idx = (size_t)(nmath::prng_c(0.0, 1.0) * (nmath::scalar_t)lights.size());
                if (light_idx >= lights.size()) light_idx = lights.size() - 1;
                const area_light_t &light = lights[light_idx];

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
            nmath::Vector3f wo = sample_cosine_hemisphere(hit_record.normal, pdf);
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
