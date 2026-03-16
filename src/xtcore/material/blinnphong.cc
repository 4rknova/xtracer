#include <algorithm>
#include <nmath/sample.h>
#include <nmath/prng.h>
#include <nimg/luminance.h>
#include <xtcore/math/sampling_util.h>
#include "macro.h"
#include "blinnphong.h"

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

inline nmath::Vector3f blinn_shading_normal(const BlinnPhong *mat, const hit_record_t &hit_record)
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

inline void lobe_weights(const xtcore::asset::IMaterial *m,
                         const xtcore::hit_record_t &hit,
                         nimg::ColorRGBf &kd,
                         nimg::ColorRGBf &ks,
                         nmath::scalar_t &w_diff,
                         nmath::scalar_t &w_spec)
{
    kd = m->get_sample(MAT_SAMPLER_DIFFUSE, hit.texcoord);
    ks = m->get_sample(MAT_SAMPLER_SPECULAR, hit.texcoord);

    const nmath::scalar_t ld = std::max((nmath::scalar_t)0.0, safe_luma(kd));
    const nmath::scalar_t ls = std::max((nmath::scalar_t)0.0, safe_luma(ks));
    const nmath::scalar_t sum = ld + ls;

    if (sum <= (nmath::scalar_t)EPSILON) {
        w_diff = 1.0;
        w_spec = 0.0;
        return;
    }

    const nmath::scalar_t refl = clamp_scalar((nmath::scalar_t)m->get_scalar(MAT_SCALART_REFLECTANCE), 0.0, 1.0);
    if (refl > (nmath::scalar_t)EPSILON && refl < (nmath::scalar_t)(1.0 - EPSILON)) {
        w_spec = refl;
        w_diff = (nmath::scalar_t)1.0 - refl;
    } else {
        w_diff = ld / sum;
        w_spec = ls / sum;
    }
}

} // namespace

bool BlinnPhong::shade(
            ColorRGBf    &intensity
    , const ICamera      *camera
    , const emitter_t    *emitter
    , const hit_record_t &hit_record) const
{
    Vector3f light_dir = (emitter->position - hit_record.point).normalized();
    const Vector3f n = blinn_shading_normal(this, hit_record);

    nmath::scalar_t d = nmath::max(dot(light_dir, n), 0);

    Vector3f ray = camera->position - hit_record.point;
    ray.normalize();

    Vector3f r = light_dir + ray;
    r.normalize();

    nmath::scalar_t rmv = nmath::max(dot(r, n), 0);

    intensity = emitter->intensity;
    intensity *= (d * get_sample(MAT_SAMPLER_DIFFUSE, hit_record.texcoord))
            + (get_sample(MAT_SAMPLER_SPECULAR, hit_record.texcoord) * pow((long double)rmv, (long double)get_scalar(MAT_SCALART_EXPONENT)));

    return true;
}

bool BlinnPhong::sample_path(
            hit_result_t &hit_result
    , const hit_record_t &hit_record
) const
{
    const Vector3f n = blinn_shading_normal(this, hit_record);
    const Vector3f ng = hit_record.normal.normalized();
    hit_result.ray.origin = hit_record.point + ng * EPSILON;

    scalar_t s = get_scalar("reflectance");
    if (s < 0.0f) s = 0.0f;
    if (s > 1.0f) s = 1.0f;
    scalar_t k = nmath::prng_c(0.0f, 1.0f); // Reflection probability
    const scalar_t p_spec = s;
    const scalar_t p_diff = 1.0f - s;

    if (k > s) {
        const scalar_t inv_p = (p_diff > EPSILON) ? (1.0f / p_diff) : 0.0f;
        hit_result.intensity = get_sample("diffuse", hit_record.texcoord) * inv_p;
        hit_result.ray.direction = nmath::sample::diffuse(n);
    }
    else {
        const scalar_t inv_p = (p_spec > EPSILON) ? (1.0f / p_spec) : 0.0f;
        hit_result.intensity = get_sample("specular", hit_record.texcoord) * inv_p;
        scalar_t exp = get_scalar("exponent");
        hit_result.ray.direction = nmath::sample::lobe(n, -hit_record.incident_direction, exp).normalized();
/*
        hit_result.ray.direction = (hit_result.ray.direction + 1.0) * 0.5;
        nimg::ColorRGBf resc(hit_result.ray.direction.x, hit_result.ray.direction.y, hit_result.ray.direction.z);
        hit_result.intensity = resc;
        return false;
*/
    }

    return true;
}

bool BlinnPhong::bsdf_eval(
            const hit_record_t &hit_record
    , const Vector3f &wo
    , const Vector3f &wi
    , ColorRGBf &f
    , scalar_t &pdf
) const
{
    const nmath::Vector3f n = blinn_shading_normal(this, hit_record);
    const nmath::Vector3f wo_n = wo.normalized();
    const nmath::Vector3f wi_n = wi.normalized();

    const nmath::scalar_t cos_i = std::max((nmath::scalar_t)0.0, nmath::dot(n, wi_n));
    const nmath::scalar_t cos_o = std::max((nmath::scalar_t)0.0, nmath::dot(n, wo_n));
    if (cos_i <= (nmath::scalar_t)EPSILON || cos_o <= (nmath::scalar_t)EPSILON) {
        f = ColorRGBf(0.0f, 0.0f, 0.0f);
        pdf = 0.0f;
        return false;
    }

    nimg::ColorRGBf kd, ks;
    nmath::scalar_t w_diff = 1.0;
    nmath::scalar_t w_spec = 0.0;
    lobe_weights(this, hit_record, kd, ks, w_diff, w_spec);

    f = ColorRGBf(0.0f, 0.0f, 0.0f);
    pdf = 0.0f;

    if (safe_luma(kd) > (nmath::scalar_t)EPSILON) {
        f += kd * ((nmath::scalar_t)1.0 / nmath::PI);
        pdf += w_diff * (cos_i / nmath::PI);
    }

    if (safe_luma(ks) > (nmath::scalar_t)EPSILON) {
        const nmath::scalar_t exp = std::max((nmath::scalar_t)1.0, (nmath::scalar_t)get_scalar(MAT_SCALART_EXPONENT));
        const nmath::Vector3f r = wo_n.reflected(n).normalized();
        const nmath::scalar_t ca = std::max((nmath::scalar_t)0.0, nmath::dot(r, wi_n));
        if (ca > (nmath::scalar_t)EPSILON) {
            const nmath::scalar_t phong_brdf = ((exp + (nmath::scalar_t)2.0) / ((nmath::scalar_t)2.0 * nmath::PI)) * nmath_pow(ca, exp);
            const nmath::scalar_t phong_pdf = ((exp + (nmath::scalar_t)1.0) / ((nmath::scalar_t)2.0 * nmath::PI)) * nmath_pow(ca, exp);
            f += ks * phong_brdf;
            pdf += w_spec * phong_pdf;
        }
    }

    return (pdf > (nmath::scalar_t)EPSILON) && (safe_luma(f) > (nmath::scalar_t)EPSILON);
}

bool BlinnPhong::bsdf_sample(
            const hit_record_t &hit_record
    , const Vector3f &wo
    , Vector3f &wi
    , ColorRGBf &f
    , scalar_t &pdf
) const
{
    const nmath::Vector3f n = blinn_shading_normal(this, hit_record);
    nimg::ColorRGBf kd, ks;
    nmath::scalar_t w_diff = 1.0;
    nmath::scalar_t w_spec = 0.0;
    lobe_weights(this, hit_record, kd, ks, w_diff, w_spec);

    const nmath::scalar_t k = nmath::prng_c(0.0, 1.0);
    if (w_spec > (nmath::scalar_t)EPSILON && k < w_spec) {
        const nmath::scalar_t exp = std::max((nmath::scalar_t)1.0, (nmath::scalar_t)get_scalar(MAT_SCALART_EXPONENT));
        nmath::scalar_t tmp_pdf = 0.0;
        wi = xtcore::math::sampling::sample_power_cosine_lobe(wo.normalized().reflected(n), exp, tmp_pdf);
    } else {
        nmath::scalar_t tmp_pdf = 0.0;
        wi = xtcore::math::sampling::sample_cosine_hemisphere(n, tmp_pdf);
    }

    return bsdf_eval(hit_record, wo, wi, f, pdf);
}

        } /* namespace material */
    } /* namespace asset */
} /* namespace xtcore */
