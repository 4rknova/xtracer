#include <algorithm>
#include <cmath>
#include <map>
#include <vector>

#include <nmath/precision.h>
#include <nmath/prng.h>
#include <nimg/luminance.h>

#include <xtcore/aa.h>
#include <xtcore/matdefs.h>
#include <xtcore/material.h>
#include <xtcore/material/boundary.h>
#include <xtcore/medium_util.h>
#include <xtcore/math/sphere.h>
#include <xtcore/math/triangle.h>
#include <xtcore/math/sampling_util.h>
#include <xtcore/mesh.h>
#include <xtcore/tile.h>

#include "integrator.h"

namespace xtcore {
    namespace integrator {
        namespace pathtracer_bdpt {

namespace {

typedef Integrator::area_light_t area_light_t;

// ---------------------------------------------------------------------------
// Geometry helpers (shared with pathtracer_mis pattern)
// ---------------------------------------------------------------------------

inline nmath::scalar_t triangle_area(const nmath::Vector3f &a,
                                     const nmath::Vector3f &b,
                                     const nmath::Vector3f &c)
{
    return (nmath::scalar_t)0.5 * nmath::cross(b - a, c - a).length();
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
        for (const auto &t : mesh->triangles())
            area += triangle_area(t.v[0], t.v[1], t.v[2]);
        return area;
    }
    return 0.0;
}

inline nmath::scalar_t safe_luma(const nimg::ColorRGBf &c)
{
    return (nmath::scalar_t)nimg::eval::luminance(c);
}

inline nmath::scalar_t power_heuristic(nmath::scalar_t p_a, nmath::scalar_t p_b)
{
    const nmath::scalar_t a2 = p_a * p_a, b2 = p_b * p_b;
    const nmath::scalar_t d = a2 + b2;
    return (d <= (nmath::scalar_t)EPSILON) ? 0.0 : a2 / d;
}

inline bool unoccluded(xtcore::render::context_t *ctx,
                       const nmath::Vector3f &a,
                       const nmath::Vector3f &b,
                       HASH_ID light_id = HASH_ID_INVALID)
{
    const nmath::Vector3f v = b - a;
    const nmath::scalar_t dist = v.length();
    if (dist <= (nmath::scalar_t)EPSILON) return false;
    const nmath::Vector3f dir = v / dist;

    xtcore::Ray ray;
    ray.origin    = a + dir * (nmath::scalar_t)EPSILON;
    ray.direction = dir;

    for (int step = 0; step < 16; ++step) {
        xtcore::hit_record_t rec;
        if (!ctx->scene->intersection(ray, rec)) return true;
        if (rec.t >= dist - (nmath::scalar_t)1e-4) {
            return (light_id == HASH_ID_INVALID) || (rec.id_object == light_id);
        }
        if (rec.id_object == light_id) return true;
        const xtcore::asset::IMaterial *mat = ctx->scene->get_material(rec.id_object);
        if (!mat || dynamic_cast<const xtcore::asset::material::Boundary *>(mat) == nullptr)
            return false;
        ray.origin = rec.point + dir * (nmath::scalar_t)EPSILON;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Barycentric / light point sampling helpers
// ---------------------------------------------------------------------------

inline void sample_barycentric(nmath::scalar_t &b0, nmath::scalar_t &b1, nmath::scalar_t &b2)
{
    const nmath::scalar_t u  = nmath::prng_c(0.0, 1.0);
    const nmath::scalar_t v  = nmath::prng_c(0.0, 1.0);
    const nmath::scalar_t su = nmath_sqrt(std::max((nmath::scalar_t)0.0, u));
    b0 = (nmath::scalar_t)1.0 - su;
    b1 = su * ((nmath::scalar_t)1.0 - v);
    b2 = su * v;
}

inline bool sample_light_surface(const area_light_t &light,
                                 nmath::Vector3f &point,
                                 nmath::Vector3f &normal,
                                 nmath::Vector3f &texcoord,
                                 nmath::scalar_t &pdf_area)
{
    if (!light.surface || light.area <= (nmath::scalar_t)EPSILON) return false;

    const xtcore::surface::Sphere *sp = dynamic_cast<const xtcore::surface::Sphere *>(light.surface);
    if (sp) {
        point   = sp->point_sample();
        normal  = (point - sp->origin).normalized();
        const nmath::scalar_t uvsx = (sp->uv_scale.x != 0.0f ? sp->uv_scale.x : 1.0f);
        const nmath::scalar_t uvsy = (sp->uv_scale.y != 0.0f ? sp->uv_scale.y : 1.0f);
        texcoord  = nmath::Vector3f(nmath_asin(normal.x / uvsx) / nmath::PI + 0.5,
                                    nmath_asin(normal.y / uvsy) / nmath::PI + 0.5, 0.0);
        pdf_area  = 1.0 / light.area;
        return true;
    }

    const xtcore::surface::Triangle *tr = dynamic_cast<const xtcore::surface::Triangle *>(light.surface);
    if (tr) {
        nmath::scalar_t b0, b1, b2;
        sample_barycentric(b0, b1, b2);
        point    = tr->v[0] * b0 + tr->v[1] * b1 + tr->v[2] * b2;
        normal   = (tr->n[0] * b0 + tr->n[1] * b1 + tr->n[2] * b2);
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
        const xtcore::surface::Triangle *sel = &tris[0];
        for (const auto &t : tris) {
            accum += triangle_area(t.v[0], t.v[1], t.v[2]);
            if (pick <= accum) { sel = &t; break; }
        }
        nmath::scalar_t b0, b1, b2;
        sample_barycentric(b0, b1, b2);
        point   = sel->v[0] * b0 + sel->v[1] * b1 + sel->v[2] * b2;
        normal  = sel->n[0] * b0 + sel->n[1] * b1 + sel->n[2] * b2;
        if (normal.length() <= (nmath::scalar_t)EPSILON) normal = sel->calc_normal();
        normal.normalize();
        nmath::Vector2f uv = sel->tc[0] * b0 + sel->tc[1] * b1 + sel->tc[2] * b2;
        const nmath::scalar_t uvsx = (mesh->uv_scale.x != 0.0f ? mesh->uv_scale.x : 1.0f);
        const nmath::scalar_t uvsy = (mesh->uv_scale.y != 0.0f ? mesh->uv_scale.y : 1.0f);
        texcoord = nmath::Vector3f(uv.x * uvsx, uv.y * uvsy, 0.0);
        pdf_area = 1.0 / light.area;
        return true;
    }
    return false;
}

inline void collect_area_lights(xtcore::render::context_t *ctx,
                                std::vector<area_light_t> &lights)
{
    lights.clear();
    if (!ctx) return;
    for (auto it = ctx->scene->m_objects.begin(); it != ctx->scene->m_objects.end(); ++it) {
        const HASH_ID id = it->first;
        const xtcore::asset::ISurface  *surface  = ctx->scene->get_surface(id);
        const xtcore::asset::IMaterial *material = ctx->scene->get_material(id);
        if (!surface || !material || !material->is_emissive()) continue;
        const nmath::scalar_t area = light_area(surface);
        if (area <= (nmath::scalar_t)EPSILON) continue;
        const nimg::ColorRGBf le = material->get_sample(MAT_SAMPLER_EMISSIVE, nmath::Vector3f(0,0,0));
        area_light_t l;
        l.object_id     = id;
        l.surface       = surface;
        l.material      = material;
        l.area          = area;
        l.select_weight = area * std::max((nmath::scalar_t)1e-4, safe_luma(le));
        lights.push_back(l);
    }
}

inline void build_light_cdf(const std::vector<area_light_t> &lights,
                            std::vector<nmath::scalar_t> &cdf,
                            nmath::scalar_t &total,
                            std::map<HASH_ID, nmath::scalar_t> &select_pdf)
{
    cdf.clear(); select_pdf.clear(); total = 0.0;
    for (const auto &l : lights) { total += l.select_weight; cdf.push_back(total); }
    if (total <= (nmath::scalar_t)EPSILON) return;
    for (const auto &l : lights) select_pdf[l.object_id] = l.select_weight / total;
}

inline bool pick_light(const std::vector<nmath::scalar_t> &cdf,
                       nmath::scalar_t total, size_t &out_idx)
{
    if (cdf.empty() || total <= (nmath::scalar_t)EPSILON) return false;
    const nmath::scalar_t u = nmath::prng_c(0.0, 1.0) * total;
    out_idx = (size_t)(std::lower_bound(cdf.begin(), cdf.end(), u) - cdf.begin());
    if (out_idx >= cdf.size()) out_idx = cdf.size() - 1;
    return true;
}

} // namespace

// ---------------------------------------------------------------------------
// Setup / teardown
// ---------------------------------------------------------------------------

void Integrator::setup_auxiliary()
{
    m_lights.clear(); m_light_cdf.clear();
    m_light_index_by_objid.clear(); m_light_select_pdf.clear();
    m_light_weight_sum = 0.0;

    collect_area_lights(ctx, m_lights);
    for (size_t i = 0; i < m_lights.size(); ++i)
        m_light_index_by_objid[m_lights[i].object_id] = i;
    build_light_cdf(m_lights, m_light_cdf, m_light_weight_sum, m_light_select_pdf);
}

void Integrator::clean_auxiliary()
{
    m_lights.clear(); m_light_cdf.clear();
    m_light_index_by_objid.clear(); m_light_select_pdf.clear();
    m_light_weight_sum = 0.0;
}

// ---------------------------------------------------------------------------
// Camera subpath
// Vertices: [0] = first surface hit, [1] = second, ...
// ---------------------------------------------------------------------------

int Integrator::trace_camera_path(const xtcore::Ray &primary_ray, size_t max_depth,
                                  std::vector<PathVertex> &path,
                                  nimg::ColorRGBf &env_contribution)
{
    path.clear();
    env_contribution = nimg::ColorRGBf(0, 0, 0);
    if (max_depth == 0) return 0;

    xtcore::Ray ray = primary_ray;
    nimg::ColorRGBf throughput(1, 1, 1);
    nmath::scalar_t ior = 1.0;

    for (size_t bounce = 0; bounce < max_depth; ++bounce) {
        xtcore::hit_record_t hr;
        if (!ctx->scene->intersection(ray, hr)) {
            env_contribution = throughput * ctx->scene->sample_environment(ray.direction);
            break;
        }

        hr.ior = ior;
        const xtcore::asset::IMaterial *mat = ctx->scene->get_material(hr.id_object);
        if (!mat) break;

        const nmath::Vector3f wo_vertex = (-ray.direction).normalized();

        PathVertex v;
        v.point      = hr.point;
        v.normal     = hr.normal;
        v.texcoord   = hr.texcoord;
        v.throughput = throughput;
        v.pdf_fwd    = 1.0;
        v.pdf_rev    = 0.0;
        v.material   = mat;
        v.obj_id     = hr.id_object;
        v.wo         = wo_vertex;
        v.is_delta   = mat->bsdf_is_delta();
        v.on_light   = mat->is_emissive();
        path.push_back(v);

        if (mat->is_emissive()) break;  // stop at emitter

        nmath::Vector3f wo = wo_vertex;
        nmath::Vector3f wi;
        nimg::ColorRGBf f;
        nmath::scalar_t pdf = 0.0;

        bool sampled = false;
        if (!mat->bsdf_is_delta()) {
            sampled = mat->bsdf_sample(hr, wo, wi, f, pdf);
        }

        if (!sampled) {
            // delta BSDF — use sample_path
            xtcore::hit_result_t nxt;
            nxt.ior = ior;
            if (!mat->sample_path(nxt, hr)) break;
            throughput *= nxt.intensity;
            ray = nxt.ray;
            ior = nxt.ior;
            path.back().is_delta = true;
            continue;
        }

        const nmath::scalar_t cos_t = nmath_abs(nmath::dot(hr.normal, wi));
        if (cos_t <= (nmath::scalar_t)EPSILON || pdf <= (nmath::scalar_t)EPSILON) break;
        path.back().pdf_fwd = pdf;
        throughput *= f * (cos_t / pdf);

        // Russian roulette after 3 bounces
        if (bounce >= 3) {
            const nmath::scalar_t rr = std::max((nmath::scalar_t)0.1,
                                                std::min((nmath::scalar_t)0.95,
                                                         (nmath::scalar_t)nimg::eval::luminance(throughput)));
            if (nmath::prng_c(0.0, 1.0) > rr) break;
            throughput *= (1.0 / rr);
        }

        const nmath::scalar_t normal_side = nmath::dot(hr.normal, wi) >= 0 ? 1.0 : -1.0;
        ray.origin    = hr.point + hr.normal * (normal_side * (nmath::scalar_t)EPSILON);
        ray.direction = wi;
    }

    return (int)path.size();
}

// ---------------------------------------------------------------------------
// Light subpath
// Vertices: [0] = point on the light surface, [1] = first scatter, ...
// ---------------------------------------------------------------------------

int Integrator::trace_light_path(size_t max_depth, std::vector<PathVertex> &path)
{
    path.clear();
    if (max_depth == 0 || m_lights.empty() || m_light_weight_sum <= (nmath::scalar_t)EPSILON)
        return 0;

    size_t light_idx = 0;
    if (!pick_light(m_light_cdf, m_light_weight_sum, light_idx)) return 0;
    const area_light_t &light = m_lights[light_idx];
    const nmath::scalar_t p_select = light.select_weight / m_light_weight_sum;

    nmath::Vector3f lp, ln, ltc;
    nmath::scalar_t pdf_area = 0.0;
    if (!sample_light_surface(light, lp, ln, ltc, pdf_area)) return 0;

    // Le at sampled point
    const nimg::ColorRGBf le = light.material->get_sample(MAT_SAMPLER_EMISSIVE, ltc);

    // Emit a cosine-weighted ray from the light surface
    nmath::scalar_t pdf_dir = 0.0;
    const nmath::Vector3f emit_dir = xtcore::math::sampling::sample_cosine_hemisphere(ln, pdf_dir);
    if (pdf_dir <= (nmath::scalar_t)EPSILON) return 0;

    // Light origin vertex
    PathVertex lv;
    lv.point      = lp;
    lv.normal     = ln;
    lv.texcoord   = ltc;
    lv.throughput = le;          // Le at origin; divided by selection+area PDF
    lv.pdf_fwd    = p_select * pdf_area;
    lv.pdf_rev    = 0.0;
    lv.material   = light.material;
    lv.obj_id     = light.object_id;
    lv.wo         = ln;   // light origin: "outgoing" is the surface normal (emission direction)
    lv.is_delta   = false;
    lv.on_light   = true;

    // Light origin throughput = Le / p_origin.
    // The cos_emit/pdf_dir factor is for propagating to lv[1]+, not lv[0].
    const nmath::scalar_t origin_pdf = p_select * pdf_area;
    const nimg::ColorRGBf origin_throughput = le * (1.0 / std::max((nmath::scalar_t)EPSILON, origin_pdf));

    const nmath::scalar_t cos_emit = std::max((nmath::scalar_t)0.0, nmath::dot(ln, emit_dir));
    nimg::ColorRGBf throughput = origin_throughput * (cos_emit / pdf_dir);

    lv.throughput = origin_throughput;  // lv[0] carries Le/p_origin only
    path.push_back(lv);

    if (max_depth == 1) return 1;

    xtcore::Ray ray;
    ray.origin    = lp + emit_dir * (nmath::scalar_t)EPSILON;
    ray.direction = emit_dir;
    nmath::scalar_t ior = 1.0;

    for (size_t bounce = 1; bounce < max_depth; ++bounce) {
        xtcore::hit_record_t hr;
        if (!ctx->scene->intersection(ray, hr)) break;

        hr.ior = ior;
        const xtcore::asset::IMaterial *mat = ctx->scene->get_material(hr.id_object);
        if (!mat || mat->is_emissive()) break;

        const nmath::Vector3f wo_vertex = (-ray.direction).normalized();

        PathVertex v;
        v.point      = hr.point;
        v.normal     = hr.normal;
        v.texcoord   = hr.texcoord;
        v.throughput = throughput;
        v.pdf_fwd    = 1.0;
        v.pdf_rev    = 0.0;
        v.material   = mat;
        v.obj_id     = hr.id_object;
        v.wo         = wo_vertex;
        v.is_delta   = mat->bsdf_is_delta();
        v.on_light   = false;
        path.push_back(v);

        nmath::Vector3f wo = wo_vertex;
        nmath::Vector3f wi;
        nimg::ColorRGBf f;
        nmath::scalar_t pdf = 0.0;

        bool sampled = false;
        if (!mat->bsdf_is_delta()) {
            sampled = mat->bsdf_sample(hr, wo, wi, f, pdf);
        }

        if (!sampled) {
            xtcore::hit_result_t nxt;
            nxt.ior = ior;
            if (!mat->sample_path(nxt, hr)) break;
            throughput *= nxt.intensity;
            ray = nxt.ray;
            ior = nxt.ior;
            path.back().is_delta = true;
            continue;
        }

        const nmath::scalar_t cos_t = nmath_abs(nmath::dot(hr.normal, wi));
        if (cos_t <= (nmath::scalar_t)EPSILON || pdf <= (nmath::scalar_t)EPSILON) break;
        path.back().pdf_fwd = pdf;
        throughput *= f * (cos_t / pdf);

        if (bounce >= 3) {
            const nmath::scalar_t rr = std::max((nmath::scalar_t)0.1,
                                                std::min((nmath::scalar_t)0.95,
                                                         (nmath::scalar_t)nimg::eval::luminance(throughput)));
            if (nmath::prng_c(0.0, 1.0) > rr) break;
            throughput *= (1.0 / rr);
        }

        const nmath::scalar_t normal_side = nmath::dot(hr.normal, wi) >= 0 ? 1.0 : -1.0;
        ray.origin    = hr.point + hr.normal * (normal_side * (nmath::scalar_t)EPSILON);
        ray.direction = wi;
    }

    return (int)path.size();
}

// ---------------------------------------------------------------------------
// connect(): evaluate one (s, t) strategy
//   s = number of camera vertices used (>= 1)
//   t = number of light  vertices used (>= 1)
//   Camera path index:  cam_path[s-1]
//   Light  path index:  lgt_path[t-1]
// ---------------------------------------------------------------------------

nimg::ColorRGBf Integrator::connect(const std::vector<PathVertex> &cam_path, int s,
                                    const std::vector<PathVertex> &lgt_path, int t)
{
    const nimg::ColorRGBf black(0, 0, 0);

    if (s < 1 || t < 1) return black;
    if (s > (int)cam_path.size() || t > (int)lgt_path.size()) return black;

    const PathVertex &cv = cam_path[s - 1];
    const PathVertex &lv = lgt_path[t - 1];

    // Skip delta vertices — they cannot be explicitly connected
    if (cv.is_delta || lv.is_delta) return black;

    // Visibility
    if (!unoccluded(ctx, cv.point, lv.point)) return black;

    const nmath::Vector3f to_light = (lv.point - cv.point);
    const nmath::scalar_t dist2    = to_light.length_squared();
    if (dist2 <= (nmath::scalar_t)EPSILON) return black;
    const nmath::Vector3f wi_cam   = to_light / nmath_sqrt(dist2);  // at camera vertex, toward light
    const nmath::Vector3f wi_lgt   = -wi_cam;                       // at light vertex, toward camera

    // Geometry term
    const nmath::scalar_t cos_cam = nmath_abs(nmath::dot(cv.normal, wi_cam));
    const nmath::scalar_t cos_lgt = nmath_abs(nmath::dot(lv.normal, wi_lgt));
    if (cos_cam <= (nmath::scalar_t)EPSILON || cos_lgt <= (nmath::scalar_t)EPSILON) return black;
    const nmath::scalar_t G = cos_cam * cos_lgt / dist2;

    auto make_hr = [](const PathVertex &v) -> xtcore::hit_record_t {
        xtcore::hit_record_t hr;
        hr.point              = v.point;
        hr.normal             = v.normal;
        hr.texcoord           = v.texcoord;
        hr.ior                = 1.0;
        hr.id_object          = v.obj_id;
        hr.incident_direction = -v.wo;
        return hr;
    };

    // BSDF at camera vertex
    nimg::ColorRGBf f_cam;
    nmath::scalar_t pdf_cam = 0.0;
    if (!cv.material->bsdf_eval(make_hr(cv), cv.wo, wi_cam, f_cam, pdf_cam)) return black;

    nimg::ColorRGBf contrib;
    if (lv.on_light) {
        // t=1: connecting directly to the light surface.
        // lv.throughput = Le/p_origin. No BSDF on an emissive surface.
        contrib = cv.throughput * f_cam * G * lv.throughput;
    } else {
        // t>1: lv is a scattered light-path vertex; evaluate its BSDF toward cv.
        nimg::ColorRGBf f_lgt;
        nmath::scalar_t pdf_lgt = 0.0;
        if (!lv.material->bsdf_eval(make_hr(lv), lv.wo, wi_lgt, f_lgt, pdf_lgt)) return black;
        contrib = cv.throughput * f_cam * G * f_lgt * lv.throughput;
    }

    if (safe_luma(contrib) <= (nmath::scalar_t)EPSILON) return black;
    return contrib;
}

// ---------------------------------------------------------------------------
// render_tile
// ---------------------------------------------------------------------------

void Integrator::render_tile(xtcore::render::tile_t *tile)
{
    xtcore::asset::ICamera *cam = ctx->active_camera();
    const size_t max_depth = ctx->params.rdepth;

    while (tile->samples.count() > 0) {
        xtcore::antialiasing::sample_rgba_t aa_sample;
        tile->samples.pop(aa_sample);

        nimg::ColorRGBAf color_pixel;
        tile->read(aa_sample.pixel.x, aa_sample.pixel.y, color_pixel);

        xtcore::Ray primary = cam->get_primary_ray(
              aa_sample.coords.x, aa_sample.coords.y
            , (float)ctx->params.width
            , (float)ctx->params.height);

        std::vector<PathVertex> cam_path, lgt_path;
        nimg::ColorRGBf env_contribution;
        trace_camera_path(primary, max_depth, cam_path, env_contribution);
        trace_light_path(max_depth, lgt_path);

        nimg::ColorRGBf radiance(0, 0, 0);

        // --- Strategy 1: camera-only paths (emit hit + environment) ---
        for (int s = 1; s <= (int)cam_path.size(); ++s) {
            const PathVertex &cv = cam_path[s - 1];
            if (cv.on_light) {
                // Direct hit of emitter: full throughput * Le
                const nimg::ColorRGBf le = cv.material->get_sample(MAT_SAMPLER_EMISSIVE, cv.texcoord);
                radiance += cv.throughput * le;
                break;
            }
        }

        // Environment: covers both primary-miss and mid-path escapes
        radiance += env_contribution;

        // --- Strategy 2: connect camera and light subpath vertices ---
        for (int s = 1; s <= (int)cam_path.size(); ++s) {
            if (cam_path[s - 1].on_light) break;  // already counted above
            for (int t = 1; t <= (int)lgt_path.size(); ++t) {
                radiance += connect(cam_path, s, lgt_path, t);
            }
        }

        color_pixel += radiance * aa_sample.weight;
        color_pixel.a(1);
        tile->write(floor(aa_sample.pixel.x), floor(aa_sample.pixel.y), color_pixel);
    }
}

        } /* namespace pathtracer_bdpt */
    } /* namespace integrator */
} /* namespace xtcore */
