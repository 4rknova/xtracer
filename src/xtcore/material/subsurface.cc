#include <algorithm>
#include <cmath>

#include <nimg/luminance.h>
#include <nmath/prng.h>
#include <xtcore/math/sampling_util.h>

#include "macro.h"
#include "subsurface.h"

namespace xtcore {
    namespace asset {
        namespace material {

namespace {

inline nmath::scalar_t clamp_scalar(nmath::scalar_t v, nmath::scalar_t lo, nmath::scalar_t hi)
{
    return std::max(lo, std::min(v, hi));
}

inline nmath::scalar_t safe_luma(const nimg::ColorRGBf &c)
{
    return (nmath::scalar_t)nimg::eval::luminance(c);
}

inline nmath::Vector3f shading_normal(const xtcore::asset::IMaterial *mat, const hit_record_t &hit_record)
{
    nmath::Vector3f n = hit_record.normal.normalized();
    if (!mat || !mat->has_sampler(MAT_SAMPLER_NORMAL)) return n;

    const nimg::ColorRGBf tex = mat->get_sample(MAT_SAMPLER_NORMAL, hit_record.texcoord);
    nmath::Vector3f tangent_space_n(
        (nmath::scalar_t)(tex.r() * 2.0f - 1.0f),
        (nmath::scalar_t)(tex.g() * 2.0f - 1.0f),
        (nmath::scalar_t)(tex.b() * 2.0f - 1.0f)
    );
    if (tangent_space_n.length() <= (nmath::scalar_t)EPSILON) return n;
    tangent_space_n.normalize();

    const nmath::Vector3f t = xtcore::math::sampling::build_tangent(n);
    nmath::Vector3f b = nmath::cross(n, t);
    if (b.length() <= (nmath::scalar_t)EPSILON) return n;
    b.normalize();

    nmath::Vector3f mapped = (t * tangent_space_n.x) + (b * tangent_space_n.y) + (n * tangent_space_n.z);
    if (mapped.length() <= (nmath::scalar_t)EPSILON) return n;
    mapped.normalize();
    if (nmath::dot(mapped, n) <= (nmath::scalar_t)0.0) return n;
    return mapped;
}

inline nimg::ColorRGBf diffuse_color(const xtcore::asset::IMaterial *mat, const hit_record_t &hit_record)
{
    if (mat->has_sampler(MAT_SAMPLER_BASE_COLOR)) return mat->get_sample(MAT_SAMPLER_BASE_COLOR, hit_record.texcoord);
    if (mat->has_sampler(MAT_SAMPLER_DIFFUSE)) return mat->get_sample(MAT_SAMPLER_DIFFUSE, hit_record.texcoord);
    return nimg::ColorRGBf(1.0f, 1.0f, 1.0f);
}

inline nimg::ColorRGBf subsurface_color(const xtcore::asset::IMaterial *mat, const hit_record_t &hit_record)
{
    if (mat->has_sampler(MAT_SAMPLER_SUBSURFACE_COLOR)) return mat->get_sample(MAT_SAMPLER_SUBSURFACE_COLOR, hit_record.texcoord);
    return diffuse_color(mat, hit_record);
}

inline nimg::ColorRGBf subsurface_radius(const xtcore::asset::IMaterial *mat, const hit_record_t &hit_record)
{
    if (mat->has_sampler(MAT_SAMPLER_SUBSURFACE_RADIUS)) return mat->get_sample(MAT_SAMPLER_SUBSURFACE_RADIUS, hit_record.texcoord);
    return nimg::ColorRGBf(0.0f, 0.0f, 0.0f);
}

inline nmath::scalar_t subsurface_weight(const xtcore::asset::IMaterial *mat)
{
    return clamp_scalar((nmath::scalar_t)mat->get_scalar(MAT_SCALART_SUBSURFACE), (nmath::scalar_t)0.0, (nmath::scalar_t)1.0);
}

inline nmath::scalar_t thickness(const xtcore::asset::IMaterial *mat)
{
    return std::max((nmath::scalar_t)0.0, (nmath::scalar_t)mat->get_scalar(MAT_SCALART_THICKNESS));
}

inline nimg::ColorRGBf subsurface_tint(const xtcore::asset::IMaterial *mat,
                                       const hit_record_t &hit_record,
                                       nmath::scalar_t path_scale)
{
    const nimg::ColorRGBf radius = subsurface_radius(mat, hit_record);
    const nmath::scalar_t base_thickness = thickness(mat);
    if (base_thickness <= (nmath::scalar_t)EPSILON) return nimg::ColorRGBf(1.0f, 1.0f, 1.0f);

    const nmath::scalar_t distance = std::max((nmath::scalar_t)0.0, base_thickness * path_scale);
    const nmath::scalar_t rr = std::max((nmath::scalar_t)1e-4, (nmath::scalar_t)radius.r());
    const nmath::scalar_t rg = std::max((nmath::scalar_t)1e-4, (nmath::scalar_t)radius.g());
    const nmath::scalar_t rb = std::max((nmath::scalar_t)1e-4, (nmath::scalar_t)radius.b());
    return nimg::ColorRGBf(
        (float)std::exp(-(double)(distance / rr)),
        (float)std::exp(-(double)(distance / rg)),
        (float)std::exp(-(double)(distance / rb)));
}

inline bool eval_subsurface(const xtcore::asset::IMaterial *mat,
                            const hit_record_t &hit_record,
                            const nmath::Vector3f &wo,
                            const nmath::Vector3f &wi,
                            nimg::ColorRGBf &f,
                            nmath::scalar_t &pdf)
{
    const nmath::Vector3f n = shading_normal(mat, hit_record);
    const nmath::Vector3f wo_n = wo.normalized();
    const nmath::Vector3f wi_n = wi.normalized();
    const nmath::scalar_t subsurface = subsurface_weight(mat);
    const nmath::scalar_t p_reflect = (nmath::scalar_t)1.0 - subsurface;
    const nmath::scalar_t p_transmit = subsurface;
    const nmath::scalar_t cos_i = nmath::dot(n, wi_n);
    const nmath::scalar_t cos_o = nmath::dot(n, wo_n);

    const bool reflect = (cos_i > (nmath::scalar_t)EPSILON) && (cos_o > (nmath::scalar_t)EPSILON);
    const bool transmit = (cos_i < (nmath::scalar_t)-EPSILON) && (cos_o > (nmath::scalar_t)EPSILON);
    if (!reflect && !transmit) {
        f = nimg::ColorRGBf(0.0f, 0.0f, 0.0f);
        pdf = 0.0f;
        return false;
    }

    if (reflect) {
        f = diffuse_color(mat, hit_record) * (p_reflect * ((nmath::scalar_t)1.0 / nmath::PI));
        pdf = p_reflect * (std::max((nmath::scalar_t)0.0, cos_i) / nmath::PI);
    } else {
        const nmath::scalar_t inv_cos_i = (nmath::scalar_t)1.0 / std::max((nmath::scalar_t)0.2, -cos_i);
        const nmath::scalar_t inv_cos_o = (nmath::scalar_t)1.0 / std::max((nmath::scalar_t)0.2, cos_o);
        const nmath::scalar_t path_scale = ((nmath::scalar_t)0.5 * (inv_cos_i + inv_cos_o));
        f = (subsurface_color(mat, hit_record) * subsurface_tint(mat, hit_record, path_scale))
          * (p_transmit * ((nmath::scalar_t)1.0 / nmath::PI));
        pdf = p_transmit * (std::max((nmath::scalar_t)0.0, -cos_i) / nmath::PI);
    }
    return (pdf > (nmath::scalar_t)EPSILON) && (safe_luma(f) > (nmath::scalar_t)EPSILON);
}

} // namespace

bool Subsurface::shade(
            ColorRGBf    &intensity
    , const ICamera      *camera
    , const emitter_t    *emitter
    , const hit_record_t &hit_record) const
{
    const nmath::Vector3f light_dir = (emitter->position - hit_record.point).normalized();
    const nmath::Vector3f view_dir = camera ? (camera->position - hit_record.point).normalized()
                                            : (-hit_record.incident_direction).normalized();
    const nmath::Vector3f n = shading_normal(this, hit_record);
    const nmath::scalar_t subsurface = subsurface_weight(this);
    const nmath::scalar_t front = std::max((nmath::scalar_t)0.0, nmath::dot(n, light_dir));
    const nmath::scalar_t back = std::max((nmath::scalar_t)0.0, nmath::dot(-n, light_dir));
    const nmath::scalar_t view_front = std::max((nmath::scalar_t)0.0, nmath::dot(n, view_dir));
    const nmath::scalar_t path_scale =
        ((nmath::scalar_t)0.5 / std::max((nmath::scalar_t)0.2, back))
      + ((nmath::scalar_t)0.5 / std::max((nmath::scalar_t)0.2, view_front));
    const nimg::ColorRGBf tint = subsurface_tint(this, hit_record, path_scale);

    intensity += emitter->intensity *
        ((diffuse_color(this, hit_record) * (((nmath::scalar_t)1.0 - subsurface) * front))
       + ((subsurface_color(this, hit_record) * tint) * (subsurface * back)));
    return true;
}

bool Subsurface::sample_path(
            hit_result_t &hit_result
    , const hit_record_t &hit_record
) const
{
    const nmath::Vector3f wo = (-hit_record.incident_direction).normalized();
    nmath::Vector3f wi;
    nimg::ColorRGBf f;
    nmath::scalar_t pdf = 0.0;
    if (!bsdf_sample(hit_record, wo, wi, f, pdf)) return false;

    const nmath::Vector3f ng = hit_record.normal.normalized();
    const nmath::scalar_t cos_theta = nmath_abs(nmath::dot(ng, wi));
    if (cos_theta <= (nmath::scalar_t)EPSILON || pdf <= (nmath::scalar_t)EPSILON) return false;

    hit_result.ray.origin = hit_record.point + wi * EPSILON;
    hit_result.ray.direction = wi;
    hit_result.intensity = f * (cos_theta / pdf);
    hit_result.ior = hit_record.ior > (nmath::scalar_t)EPSILON ? hit_record.ior : (nmath::scalar_t)1.0;
    return true;
}

bool Subsurface::bsdf_eval(
            const hit_record_t &hit_record
    , const Vector3f &wo
    , const Vector3f &wi
    , ColorRGBf &f
    , scalar_t &pdf
) const
{
    return eval_subsurface(this, hit_record, wo, wi, f, pdf);
}

bool Subsurface::bsdf_sample(
            const hit_record_t &hit_record
    , const Vector3f &wo
    , Vector3f &wi
    , ColorRGBf &f
    , scalar_t &pdf
) const
{
    const nmath::Vector3f n = shading_normal(this, hit_record);
    const nmath::scalar_t subsurface = subsurface_weight(this);
    nmath::scalar_t tmp_pdf = 0.0;
    if (nmath::prng_c(0.0, 1.0) < subsurface) {
        wi = xtcore::math::sampling::sample_cosine_hemisphere(-n, tmp_pdf);
    } else {
        wi = xtcore::math::sampling::sample_cosine_hemisphere(n, tmp_pdf);
    }
    return eval_subsurface(this, hit_record, wo, wi, f, pdf);
}

        } /* namespace material */
    } /* namespace asset */
} /* namespace xtcore */
