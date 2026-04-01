#include <algorithm>
#include <cmath>

#include <xtcore/math/sampling_util.h>

#include "macro.h"
#include "sheen.h"

namespace xtcore {
    namespace asset {
        namespace material {

namespace {

inline nmath::scalar_t clamp_scalar(nmath::scalar_t v, nmath::scalar_t lo, nmath::scalar_t hi)
{
    return std::max(lo, std::min(v, hi));
}

inline Vector3f shading_normal(const xtcore::asset::IMaterial *mat, const hit_record_t &hit_record)
{
    Vector3f n = hit_record.normal.normalized();
    if (!mat || !mat->has_sampler(MAT_SAMPLER_NORMAL)) return n;

    const ColorRGBf tex = mat->get_sample(MAT_SAMPLER_NORMAL, hit_record.texcoord);
    Vector3f tangent_space_n(
        (nmath::scalar_t)(tex.r() * 2.0f - 1.0f),
        (nmath::scalar_t)(tex.g() * 2.0f - 1.0f),
        (nmath::scalar_t)(tex.b() * 2.0f - 1.0f)
    );
    if (tangent_space_n.length() <= (nmath::scalar_t)EPSILON) return n;
    tangent_space_n.normalize();

    const Vector3f t = xtcore::math::sampling::build_tangent(n);
    Vector3f b = nmath::cross(n, t);
    if (b.length() <= (nmath::scalar_t)EPSILON) return n;
    b.normalize();

    Vector3f mapped = (t * tangent_space_n.x) + (b * tangent_space_n.y) + (n * tangent_space_n.z);
    if (mapped.length() <= (nmath::scalar_t)EPSILON) return n;
    mapped.normalize();
    if (nmath::dot(mapped, n) <= (nmath::scalar_t)0.0) return n;
    return mapped;
}

inline ColorRGBf base_color(const xtcore::asset::IMaterial *mat, const hit_record_t &hit_record)
{
    if (mat->has_sampler(MAT_SAMPLER_BASE_COLOR)) return mat->get_sample(MAT_SAMPLER_BASE_COLOR, hit_record.texcoord);
    if (mat->has_sampler(MAT_SAMPLER_DIFFUSE)) return mat->get_sample(MAT_SAMPLER_DIFFUSE, hit_record.texcoord);
    return ColorRGBf(1.0f, 1.0f, 1.0f);
}

inline ColorRGBf sheen_color(const xtcore::asset::IMaterial *mat, const hit_record_t &hit_record)
{
    if (mat->has_sampler(MAT_SAMPLER_SHEEN_COLOR)) return mat->get_sample(MAT_SAMPLER_SHEEN_COLOR, hit_record.texcoord);
    return base_color(mat, hit_record);
}

inline nmath::scalar_t sheen_strength(const xtcore::asset::IMaterial *mat)
{
    return clamp_scalar((nmath::scalar_t)mat->get_scalar(MAT_SCALART_SHEEN), (nmath::scalar_t)0.0, (nmath::scalar_t)1.0);
}

inline bool eval_sheen(const xtcore::asset::IMaterial *mat,
                       const hit_record_t &hit_record,
                       const Vector3f &wo,
                       const Vector3f &wi,
                       ColorRGBf &f,
                       nmath::scalar_t &pdf)
{
    const Vector3f n = shading_normal(mat, hit_record);
    const Vector3f wi_n = wi.normalized();
    const Vector3f wo_n = wo.normalized();
    const nmath::scalar_t cos_i = std::max((nmath::scalar_t)0.0, nmath::dot(n, wi_n));
    const nmath::scalar_t cos_o = std::max((nmath::scalar_t)0.0, nmath::dot(n, wo_n));
    if (cos_i <= (nmath::scalar_t)EPSILON || cos_o <= (nmath::scalar_t)EPSILON) {
        f = ColorRGBf(0.0f, 0.0f, 0.0f);
        pdf = 0.0f;
        return false;
    }

    Vector3f h = wi_n + wo_n;
    if (h.length() <= (nmath::scalar_t)EPSILON) {
        f = ColorRGBf(0.0f, 0.0f, 0.0f);
        pdf = 0.0f;
        return false;
    }
    h.normalize();

    const nmath::scalar_t sheen = sheen_strength(mat);
    const nmath::scalar_t fh = (nmath::scalar_t)std::pow(
        (double)((nmath::scalar_t)1.0 - std::max((nmath::scalar_t)0.0, nmath::dot(wi_n, h))),
        5.0);
    const nmath::scalar_t retro = ((nmath::scalar_t)0.25 + (nmath::scalar_t)0.75 * fh) * sheen;
    f = (base_color(mat, hit_record)
      + sheen_color(mat, hit_record) * retro) * ((nmath::scalar_t)1.0 / nmath::PI);
    pdf = cos_i / nmath::PI;
    return true;
}

} // namespace

bool Sheen::shade(
            ColorRGBf    &intensity
    , const ICamera      *camera
    , const emitter_t    *emitter
    , const hit_record_t &hit_record) const
{
    const Vector3f light_dir = (emitter->position - hit_record.point).normalized();
    const Vector3f view_dir = camera ? (camera->position - hit_record.point).normalized()
                                     : (-hit_record.incident_direction).normalized();
    ColorRGBf f;
    nmath::scalar_t pdf = 0.0;
    if (!eval_sheen(this, hit_record, view_dir, light_dir, f, pdf)) return false;
    const Vector3f n = shading_normal(this, hit_record);
    const nmath::scalar_t cos_i = std::max((nmath::scalar_t)0.0, nmath::dot(n, light_dir));
    if (cos_i <= (nmath::scalar_t)EPSILON) return false;
    intensity += emitter->intensity * f * (cos_i * nmath::PI);
    return true;
}

bool Sheen::sample_path(
            hit_result_t &hit_result
    , const hit_record_t &hit_record
) const
{
    const Vector3f wo = (-hit_record.incident_direction).normalized();
    Vector3f wi;
    ColorRGBf f;
    nmath::scalar_t pdf = 0.0;
    if (!bsdf_sample(hit_record, wo, wi, f, pdf)) return false;

    const Vector3f ng = hit_record.normal.normalized();
    const nmath::scalar_t cos_theta = nmath_abs(nmath::dot(ng, wi));
    if (cos_theta <= (nmath::scalar_t)EPSILON || pdf <= (nmath::scalar_t)EPSILON) return false;

    hit_result.ray.origin = hit_record.point + ng * EPSILON;
    hit_result.ray.direction = wi;
    hit_result.intensity = f * (cos_theta / pdf);
    return true;
}

bool Sheen::bsdf_eval(
            const hit_record_t &hit_record
    , const Vector3f &wo
    , const Vector3f &wi
    , ColorRGBf &f
    , scalar_t &pdf
) const
{
    return eval_sheen(this, hit_record, wo, wi, f, pdf);
}

bool Sheen::bsdf_sample(
            const hit_record_t &hit_record
    , const Vector3f &wo
    , Vector3f &wi
    , ColorRGBf &f
    , scalar_t &pdf
) const
{
    const Vector3f n = shading_normal(this, hit_record);
    wi = xtcore::math::sampling::sample_cosine_hemisphere(n, pdf);
    return bsdf_eval(hit_record, wo, wi, f, pdf);
}

        } /* namespace material */
    } /* namespace asset */
} /* namespace xtcore */
