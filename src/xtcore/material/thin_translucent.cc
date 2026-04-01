#include <algorithm>
#include <cmath>

#include <nimg/luminance.h>
#include <nmath/prng.h>
#include <xtcore/math/sampling_util.h>

#include "macro.h"
#include "thin_translucent.h"

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

inline nimg::ColorRGBf base_color(const xtcore::asset::IMaterial *mat, const hit_record_t &hit_record)
{
    if (mat->has_sampler(MAT_SAMPLER_BASE_COLOR)) return mat->get_sample(MAT_SAMPLER_BASE_COLOR, hit_record.texcoord);
    if (mat->has_sampler(MAT_SAMPLER_DIFFUSE)) return mat->get_sample(MAT_SAMPLER_DIFFUSE, hit_record.texcoord);
    return nimg::ColorRGBf(1.0f, 1.0f, 1.0f);
}

inline nimg::ColorRGBf translucency_color(const xtcore::asset::IMaterial *mat, const hit_record_t &hit_record)
{
    if (mat->has_sampler(MAT_SAMPLER_TRANSLUCENCY_COLOR)) return mat->get_sample(MAT_SAMPLER_TRANSLUCENCY_COLOR, hit_record.texcoord);
    if (mat->has_sampler(MAT_SAMPLER_TRANSMISSION)) return mat->get_sample(MAT_SAMPLER_TRANSMISSION, hit_record.texcoord);
    return base_color(mat, hit_record);
}

inline nmath::scalar_t translucency_strength(const xtcore::asset::IMaterial *mat)
{
    return clamp_scalar((nmath::scalar_t)mat->get_scalar(MAT_SCALART_TRANSLUCENCY), (nmath::scalar_t)0.0, (nmath::scalar_t)1.0);
}

inline nmath::scalar_t thickness(const xtcore::asset::IMaterial *mat)
{
    return std::max((nmath::scalar_t)0.0, (nmath::scalar_t)mat->get_scalar(MAT_SCALART_THICKNESS));
}

inline nmath::scalar_t wrap_term(nmath::scalar_t ndotl)
{
    return clamp_scalar(((ndotl + (nmath::scalar_t)0.35) / (nmath::scalar_t)1.35), (nmath::scalar_t)0.0, (nmath::scalar_t)1.0);
}

inline nimg::ColorRGBf thickness_tint(const xtcore::asset::IMaterial *mat,
                                      const hit_record_t &,
                                      nmath::scalar_t cos_i,
                                      nmath::scalar_t cos_o)
{
    const nmath::scalar_t t = thickness(mat);
    if (t <= (nmath::scalar_t)EPSILON) return nimg::ColorRGBf(1.0f, 1.0f, 1.0f);
    const nmath::scalar_t path_scale =
        ((nmath::scalar_t)0.5 / std::max((nmath::scalar_t)0.2, -cos_i))
      + ((nmath::scalar_t)0.5 / std::max((nmath::scalar_t)0.2, cos_o));
    const nmath::scalar_t atten = std::exp(-(double)(t * path_scale * (nmath::scalar_t)0.7));
    const nmath::scalar_t keep = (nmath::scalar_t)0.25 + (nmath::scalar_t)0.75 * atten;
    return nimg::ColorRGBf((float)keep, (float)keep, (float)keep);
}

inline bool eval_translucent(const xtcore::asset::IMaterial *mat,
                             const hit_record_t &hit_record,
                             const nmath::Vector3f &wo,
                             const nmath::Vector3f &wi,
                             nimg::ColorRGBf &f,
                             nmath::scalar_t &pdf)
{
    const nmath::Vector3f n = shading_normal(mat, hit_record);
    const nmath::Vector3f wo_n = wo.normalized();
    const nmath::Vector3f wi_n = wi.normalized();
    const nmath::scalar_t cos_i = nmath::dot(n, wi_n);
    const nmath::scalar_t cos_o = nmath::dot(n, wo_n);
    const bool reflect = (cos_i > (nmath::scalar_t)EPSILON) && (cos_o > (nmath::scalar_t)EPSILON);
    const bool transmit = (cos_i < (nmath::scalar_t)-EPSILON) && (cos_o > (nmath::scalar_t)EPSILON);
    if (!reflect && !transmit) {
        f = nimg::ColorRGBf(0.0f, 0.0f, 0.0f);
        pdf = 0.0f;
        return false;
    }

    const nmath::scalar_t translucency = translucency_strength(mat);
    const nmath::scalar_t p_reflect = (nmath::scalar_t)1.0 - translucency;
    const nmath::scalar_t p_transmit = translucency;

    if (reflect) {
        f = base_color(mat, hit_record) * (p_reflect * ((nmath::scalar_t)1.0 / nmath::PI));
        pdf = p_reflect * (std::max((nmath::scalar_t)0.0, cos_i) / nmath::PI);
    } else {
        const nmath::scalar_t wrap = wrap_term(-cos_i);
        f = (translucency_color(mat, hit_record) * thickness_tint(mat, hit_record, cos_i, cos_o))
          * (p_transmit * wrap * ((nmath::scalar_t)1.0 / nmath::PI));
        pdf = p_transmit * (std::max((nmath::scalar_t)0.0, -cos_i) / nmath::PI);
    }

    return (pdf > (nmath::scalar_t)EPSILON) && (safe_luma(f) > (nmath::scalar_t)EPSILON);
}

} // namespace

bool ThinTranslucent::shade(
            ColorRGBf    &intensity
    , const ICamera      *camera
    , const emitter_t    *emitter
    , const hit_record_t &hit_record) const
{
    const nmath::Vector3f light_dir = (emitter->position - hit_record.point).normalized();
    const nmath::Vector3f view_dir = camera ? (camera->position - hit_record.point).normalized()
                                            : (-hit_record.incident_direction).normalized();
    ColorRGBf f;
    nmath::scalar_t pdf = 0.0;
    if (!eval_translucent(this, hit_record, view_dir, light_dir, f, pdf)) return false;
    const nmath::Vector3f n = shading_normal(this, hit_record);
    const nmath::scalar_t cos_mag = nmath_abs(nmath::dot(n, light_dir));
    if (cos_mag <= (nmath::scalar_t)EPSILON) return false;
    intensity += emitter->intensity * f * (cos_mag * nmath::PI);
    return true;
}

bool ThinTranslucent::sample_path(
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

bool ThinTranslucent::bsdf_eval(
            const hit_record_t &hit_record
    , const Vector3f &wo
    , const Vector3f &wi
    , ColorRGBf &f
    , scalar_t &pdf
) const
{
    return eval_translucent(this, hit_record, wo, wi, f, pdf);
}

bool ThinTranslucent::bsdf_sample(
            const hit_record_t &hit_record
    , const Vector3f &wo
    , Vector3f &wi
    , ColorRGBf &f
    , scalar_t &pdf
) const
{
    const nmath::Vector3f n = shading_normal(this, hit_record);
    const nmath::scalar_t translucency = translucency_strength(this);
    nmath::scalar_t tmp_pdf = 0.0;
    if (nmath::prng_c(0.0, 1.0) < translucency) {
        wi = xtcore::math::sampling::sample_cosine_hemisphere(-n, tmp_pdf);
    } else {
        wi = xtcore::math::sampling::sample_cosine_hemisphere(n, tmp_pdf);
    }
    return eval_translucent(this, hit_record, wo, wi, f, pdf);
}

        } /* namespace material */
    } /* namespace asset */
} /* namespace xtcore */
