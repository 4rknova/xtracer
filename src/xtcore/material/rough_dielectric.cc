#include <algorithm>
#include <cmath>

#include <nimg/luminance.h>
#include <nmath/prng.h>
#include <xtcore/math/sampling_util.h>

#include "macro.h"
#include "rough_dielectric.h"

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

inline nmath::scalar_t sample_scalar_sampler(const xtcore::asset::IMaterial *mat,
                                             const hit_record_t &hit_record,
                                             const char *name,
                                             nmath::scalar_t fallback)
{
    if (!mat || !mat->has_sampler(name)) return fallback;
    return (nmath::scalar_t)nimg::eval::luminance(mat->get_sample(name, hit_record.texcoord));
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

inline nimg::ColorRGBf transmission_color(const xtcore::asset::IMaterial *mat, const hit_record_t &hit_record)
{
    if (mat->has_sampler(MAT_SAMPLER_TRANSMISSION)) return mat->get_sample(MAT_SAMPLER_TRANSMISSION, hit_record.texcoord);
    if (mat->has_sampler(MAT_SAMPLER_DIFFUSE)) return mat->get_sample(MAT_SAMPLER_DIFFUSE, hit_record.texcoord);
    return nimg::ColorRGBf(1.0f, 1.0f, 1.0f);
}

inline nimg::ColorRGBf absorption_color(const xtcore::asset::IMaterial *mat, const hit_record_t &hit_record)
{
    if (mat->has_sampler(MAT_SAMPLER_ABSORPTION_COLOR)) return mat->get_sample(MAT_SAMPLER_ABSORPTION_COLOR, hit_record.texcoord);
    return nimg::ColorRGBf(1.0f, 1.0f, 1.0f);
}

inline nmath::scalar_t material_roughness(const xtcore::asset::IMaterial *mat, const hit_record_t &hit_record)
{
    const nmath::scalar_t scalar_value = (nmath::scalar_t)mat->get_scalar(MAT_SCALART_ROUGHNESS);
    return clamp_scalar(sample_scalar_sampler(mat, hit_record, MAT_SAMPLER_ROUGHNESS, scalar_value), (nmath::scalar_t)0.02, (nmath::scalar_t)1.0);
}

inline nmath::scalar_t material_ior(const xtcore::asset::IMaterial *mat)
{
    const nmath::scalar_t ior = (nmath::scalar_t)mat->get_scalar(MAT_SCALART_IOR);
    return (ior > (nmath::scalar_t)1.0) ? ior : (nmath::scalar_t)1.5;
}

inline nmath::scalar_t absorption_distance(const xtcore::asset::IMaterial *mat)
{
    return std::max((nmath::scalar_t)0.0, (nmath::scalar_t)mat->get_scalar(MAT_SCALART_ABSORPTION_DISTANCE));
}

inline nimg::ColorRGBf apply_exit_absorption(const xtcore::asset::IMaterial *mat,
                                             const hit_record_t &hit_record,
                                             const nmath::Vector3f &shading_n,
                                             const nmath::Vector3f &wi,
                                             const nimg::ColorRGBf &transmission)
{
    const nmath::scalar_t slab_thickness = absorption_distance(mat);
    if (slab_thickness <= (nmath::scalar_t)EPSILON) return transmission;

    const nimg::ColorRGBf tint = absorption_color(mat, hit_record);
    const nmath::scalar_t cos_exit = std::max((nmath::scalar_t)0.05, nmath_abs(nmath::dot(shading_n.normalized(), wi.normalized())));
    const nmath::scalar_t path_length = slab_thickness / cos_exit;
    const nmath::scalar_t distance_scale = path_length / std::max((nmath::scalar_t)EPSILON, slab_thickness);

    const nmath::scalar_t r = std::pow(clamp_scalar((nmath::scalar_t)tint.r(), (nmath::scalar_t)0.0001, (nmath::scalar_t)1.0), distance_scale);
    const nmath::scalar_t g = std::pow(clamp_scalar((nmath::scalar_t)tint.g(), (nmath::scalar_t)0.0001, (nmath::scalar_t)1.0), distance_scale);
    const nmath::scalar_t b = std::pow(clamp_scalar((nmath::scalar_t)tint.b(), (nmath::scalar_t)0.0001, (nmath::scalar_t)1.0), distance_scale);
    return transmission * nimg::ColorRGBf((float)r, (float)g, (float)b);
}

inline nmath::scalar_t lobe_exponent_from_roughness(nmath::scalar_t roughness)
{
    const nmath::scalar_t r = clamp_scalar(roughness, (nmath::scalar_t)0.02, (nmath::scalar_t)1.0);
    return std::max((nmath::scalar_t)2.0, ((nmath::scalar_t)2.0 / (r * r)) - (nmath::scalar_t)2.0);
}

inline bool use_pragmatic_frosted_lobes(nmath::scalar_t roughness)
{
    return roughness >= (nmath::scalar_t)0.12;
}

inline nmath::scalar_t reflection_pdf(const nmath::Vector3f &n,
                                      const nmath::Vector3f &wo,
                                      const nmath::Vector3f &wi,
                                      nmath::scalar_t roughness)
{
    const nmath::Vector3f h = (wo + wi).normalized();
    const nmath::scalar_t voh = std::max((nmath::scalar_t)EPSILON, nmath_abs(nmath::dot(wo, h)));
    const nmath::scalar_t d = xtcore::math::sampling::ggx_ndf(n, h, roughness);
    const nmath::scalar_t nh = std::max((nmath::scalar_t)0.0, nmath::dot(n, h));
    return (d * nh) / ((nmath::scalar_t)4.0 * voh);
}

inline void dielectric_media(const hit_record_t &hit_record,
                             const xtcore::asset::IMaterial *mat,
                             const nmath::Vector3f &wo,
                             nmath::scalar_t &eta_i,
                             nmath::scalar_t &eta_t)
{
    const nmath::scalar_t current_ior = (hit_record.ior > (nmath::scalar_t)EPSILON) ? hit_record.ior : (nmath::scalar_t)1.0;
    const nmath::scalar_t mat_ior = material_ior(mat);
    const nmath::scalar_t cos_g = nmath::dot(hit_record.normal.normalized(), wo.normalized());

    eta_i = current_ior;
    eta_t = (cos_g >= (nmath::scalar_t)0.0) ? mat_ior : (nmath::scalar_t)1.0;
}

inline bool rough_dielectric_eval_impl(const xtcore::asset::IMaterial *mat,
                                       const hit_record_t &hit_record,
                                       const nmath::Vector3f &wo,
                                       const nmath::Vector3f &wi,
                                       nimg::ColorRGBf &f,
                                       nmath::scalar_t &pdf)
{
    const nmath::Vector3f ng = hit_record.normal.normalized();
    nmath::Vector3f n = shading_normal(mat, hit_record);
    const nmath::Vector3f wo_n = wo.normalized();
    const nmath::Vector3f wi_n = wi.normalized();
    if (nmath::dot(n, wo_n) < (nmath::scalar_t)0.0) n = -n;

    const nmath::scalar_t cos_o = nmath_abs(nmath::dot(n, wo_n));
    const nmath::scalar_t cos_i = nmath_abs(nmath::dot(n, wi_n));
    if (cos_o <= (nmath::scalar_t)EPSILON || cos_i <= (nmath::scalar_t)EPSILON) {
        f = nimg::ColorRGBf(0.0f, 0.0f, 0.0f);
        pdf = 0.0f;
        return false;
    }

    const nmath::scalar_t roughness = material_roughness(mat, hit_record);
    const nimg::ColorRGBf base_trans = transmission_color(mat, hit_record)
                                     * clamp_scalar((nmath::scalar_t)mat->get_scalar(MAT_SCALART_TRANSPARENCY), (nmath::scalar_t)0.0, (nmath::scalar_t)1.0);

    nmath::scalar_t eta_i = 1.0;
    nmath::scalar_t eta_t = 1.5;
    dielectric_media(hit_record, mat, wo_n, eta_i, eta_t);

    const bool reflect = (nmath::dot(ng, wo_n) * nmath::dot(ng, wi_n)) > (nmath::scalar_t)0.0;
    const nmath::scalar_t F_surface = xtcore::math::sampling::fresnel_dielectric(
        std::max((nmath::scalar_t)0.0, nmath_abs(nmath::dot(n, wo_n))),
        eta_i,
        eta_t);
    const nimg::ColorRGBf trans = apply_exit_absorption(mat, hit_record, n, wi_n, base_trans);

    if (use_pragmatic_frosted_lobes(roughness)) {
        const nmath::scalar_t exponent = lobe_exponent_from_roughness(roughness);
        const nmath::Vector3f reflect_axis = wo_n.reflected(n).normalized();
        nmath::Vector3f transmit_axis = (-wo_n).refracted(n, eta_i, eta_t).normalized();
        if (transmit_axis.length() <= (nmath::scalar_t)EPSILON) transmit_axis = reflect_axis;

        const nmath::scalar_t p_reflect = std::max((nmath::scalar_t)0.02, std::min((nmath::scalar_t)0.98, F_surface));
        const nmath::scalar_t p_transmit = std::max((nmath::scalar_t)EPSILON, (nmath::scalar_t)1.0 - p_reflect);
        const nmath::scalar_t pdf_reflect = xtcore::math::sampling::power_cosine_lobe_pdf(reflect_axis, wi_n, exponent);
        const nmath::scalar_t pdf_transmit = xtcore::math::sampling::power_cosine_lobe_pdf(transmit_axis, wi_n, exponent);

        const nmath::scalar_t brdf_pdf_ratio = (exponent + (nmath::scalar_t)2.0) / (exponent + (nmath::scalar_t)1.0);

        if (reflect) {
            pdf = p_reflect * pdf_reflect;
            if (pdf <= (nmath::scalar_t)EPSILON) {
                f = nimg::ColorRGBf(0.0f, 0.0f, 0.0f);
                return false;
            }
            // f = F * (exp+2)/(2π) * ca^exp  =  F * brdf_pdf_ratio * pdf_reflect
            const nmath::scalar_t brdf_val = F_surface * brdf_pdf_ratio * pdf_reflect;
            f = nimg::ColorRGBf((float)brdf_val, (float)brdf_val, (float)brdf_val);
            return safe_luma(f) > (nmath::scalar_t)EPSILON;
        }

        pdf = p_transmit * pdf_transmit;
        if (pdf <= (nmath::scalar_t)EPSILON) {
            f = nimg::ColorRGBf(0.0f, 0.0f, 0.0f);
            return false;
        }
        // f = (1-F) * trans * (exp+2)/(2π) * ca^exp  =  (1-F) * trans * brdf_pdf_ratio * pdf_transmit
        f = trans * (((nmath::scalar_t)1.0 - F_surface) * brdf_pdf_ratio * pdf_transmit);
        return safe_luma(f) > (nmath::scalar_t)EPSILON;
    }

    if (reflect) {
        nmath::Vector3f h = (wo_n + wi_n).normalized();
        if (h.length() <= (nmath::scalar_t)EPSILON) {
            f = nimg::ColorRGBf(0.0f, 0.0f, 0.0f);
            pdf = 0.0f;
            return false;
        }
        if (nmath::dot(n, h) < (nmath::scalar_t)0.0) h = -h;

        const nmath::scalar_t F = xtcore::math::sampling::fresnel_dielectric(nmath_abs(nmath::dot(wo_n, h)), eta_i, eta_t);
        const nmath::scalar_t D = xtcore::math::sampling::ggx_ndf(n, h, roughness);
        const nmath::scalar_t G = xtcore::math::sampling::smith_ggx_g(n, wo_n, wi_n, roughness);
        const nmath::scalar_t pr = reflection_pdf(n, wo_n, wi_n, roughness);

        f = nimg::ColorRGBf((float)F, (float)F, (float)F) * (D * G / std::max((nmath::scalar_t)EPSILON, (nmath::scalar_t)4.0 * cos_i * cos_o));
        pdf = std::max((nmath::scalar_t)EPSILON, F) * pr;
        return safe_luma(f) > (nmath::scalar_t)EPSILON;
    }

    const nmath::scalar_t eta = eta_t / std::max((nmath::scalar_t)EPSILON, eta_i);
    nmath::Vector3f h = (wo_n + wi_n * eta).normalized();
    if (h.length() <= (nmath::scalar_t)EPSILON) {
        f = nimg::ColorRGBf(0.0f, 0.0f, 0.0f);
        pdf = 0.0f;
        return false;
    }
    if (nmath::dot(n, h) < (nmath::scalar_t)0.0) h = -h;

    const nmath::scalar_t wo_h = nmath::dot(wo_n, h);
    const nmath::scalar_t wi_h = nmath::dot(wi_n, h);
    if (nmath_abs(wo_h) <= (nmath::scalar_t)EPSILON || nmath_abs(wi_h) <= (nmath::scalar_t)EPSILON) {
        f = nimg::ColorRGBf(0.0f, 0.0f, 0.0f);
        pdf = 0.0f;
        return false;
    }

    const nmath::scalar_t F = xtcore::math::sampling::fresnel_dielectric(nmath_abs(wo_h), eta_i, eta_t);
    const nmath::scalar_t D = xtcore::math::sampling::ggx_ndf(n, h, roughness);
    const nmath::scalar_t G = xtcore::math::sampling::smith_ggx_g(n, wo_n, wi_n, roughness);
    const nmath::scalar_t denom = wo_h + eta * wi_h;
    const nmath::scalar_t denom2 = denom * denom;
    if (denom2 <= (nmath::scalar_t)EPSILON) {
        f = nimg::ColorRGBf(0.0f, 0.0f, 0.0f);
        pdf = 0.0f;
        return false;
    }

    const nmath::scalar_t factor = nmath_abs((wi_h * wo_h) / std::max((nmath::scalar_t)EPSILON, cos_i * cos_o * denom2));
    const nmath::scalar_t dwm_dwi = nmath_abs((eta * eta * wi_h) / denom2);
    const nmath::scalar_t p_h = D * std::max((nmath::scalar_t)0.0, nmath::dot(n, h));

    f = trans * (((nmath::scalar_t)1.0 - F) * D * G * factor * eta * eta);
    pdf = std::max((nmath::scalar_t)EPSILON, ((nmath::scalar_t)1.0 - F)) * p_h * dwm_dwi;
    return safe_luma(f) > (nmath::scalar_t)EPSILON;
}

} // namespace

bool RoughDielectric::shade(
            ColorRGBf    &intensity
    , const ICamera      *camera
    , const emitter_t    *emitter
    , const hit_record_t &hit_record) const
{
    UNUSED(camera)
    UNUSED(emitter)
    UNUSED(hit_record)
    intensity = nimg::ColorRGBf(0.0f, 0.0f, 0.0f);
    return true;
}

bool RoughDielectric::sample_path(
            hit_result_t &hit_result
    , const hit_record_t &hit_record
) const
{
    const nmath::Vector3f ng = hit_record.normal.normalized();
    nmath::Vector3f n = shading_normal(this, hit_record);
    const nmath::Vector3f wo = (-hit_record.incident_direction).normalized();
    if (nmath::dot(n, wo) < (nmath::scalar_t)0.0) n = -n;

    nmath::scalar_t eta_i = 1.0;
    nmath::scalar_t eta_t = material_ior(this);
    dielectric_media(hit_record, this, wo, eta_i, eta_t);
    const nmath::scalar_t roughness = material_roughness(this, hit_record);
    const nimg::ColorRGBf base_trans = transmission_color(this, hit_record)
                                     * clamp_scalar((nmath::scalar_t)get_scalar(MAT_SCALART_TRANSPARENCY), (nmath::scalar_t)0.0, (nmath::scalar_t)1.0);

    if (use_pragmatic_frosted_lobes(roughness)) {
        const nmath::scalar_t exponent = lobe_exponent_from_roughness(roughness);
        const nmath::scalar_t F = xtcore::math::sampling::fresnel_dielectric(
            std::max((nmath::scalar_t)0.0, nmath_abs(nmath::dot(n, wo))),
            eta_i,
            eta_t);
        const nmath::scalar_t p_reflect = std::max((nmath::scalar_t)0.02, std::min((nmath::scalar_t)0.98, F));

        const nmath::scalar_t brdf_pdf_ratio = (exponent + (nmath::scalar_t)2.0) / (exponent + (nmath::scalar_t)1.0);

        nmath::Vector3f wi;
        if (nmath::prng_c(0.0, 1.0) < p_reflect) {
            nmath::scalar_t tmp_pdf = 0.0;
            wi = xtcore::math::sampling::sample_power_cosine_lobe(wo.reflected(n).normalized(), exponent, tmp_pdf).normalized();
            if ((nmath::dot(ng, wo) * nmath::dot(ng, wi)) <= (nmath::scalar_t)0.0) return false;
            const nmath::scalar_t cos_i = std::max((nmath::scalar_t)EPSILON, nmath_abs(nmath::dot(n, wi)));
            hit_result.ray.origin = hit_record.point + wi * EPSILON;
            hit_result.ray.direction = wi;
            hit_result.ior = eta_i;
            hit_result.intensity = nimg::ColorRGBf((float)(F * brdf_pdf_ratio * cos_i / p_reflect),
                                                   (float)(F * brdf_pdf_ratio * cos_i / p_reflect),
                                                   (float)(F * brdf_pdf_ratio * cos_i / p_reflect));
            return true;
        }

        nmath::Vector3f trans_axis = (-wo).refracted(n, eta_i, eta_t).normalized();
        if (trans_axis.length() <= (nmath::scalar_t)EPSILON) trans_axis = wo.reflected(n).normalized();
        nmath::scalar_t tmp_pdf = 0.0;
        wi = xtcore::math::sampling::sample_power_cosine_lobe(trans_axis, exponent, tmp_pdf).normalized();
        if ((nmath::dot(ng, wo) * nmath::dot(ng, wi)) > (nmath::scalar_t)0.0) return false;

        const nmath::scalar_t p_transmit = std::max((nmath::scalar_t)EPSILON, (nmath::scalar_t)1.0 - p_reflect);
        const nmath::scalar_t cos_i = std::max((nmath::scalar_t)EPSILON, nmath_abs(nmath::dot(n, wi)));
        const nimg::ColorRGBf trans = apply_exit_absorption(this, hit_record, n, wi, base_trans);
        hit_result.ray.origin = hit_record.point + wi * EPSILON;
        hit_result.ray.direction = wi;
        hit_result.ior = eta_t;
        hit_result.intensity = trans * (((nmath::scalar_t)1.0 - F) * brdf_pdf_ratio * cos_i / p_transmit);
        return true;
    }

    nmath::scalar_t h_pdf = 0.0;
    nmath::Vector3f h = xtcore::math::sampling::sample_ggx_half_vector(n, roughness, h_pdf);
    if (nmath::dot(wo, h) < (nmath::scalar_t)0.0) h = -h;

    const nmath::scalar_t F = xtcore::math::sampling::fresnel_dielectric(nmath_abs(nmath::dot(wo, h)), eta_i, eta_t);
    const nmath::scalar_t p_reflect = std::max((nmath::scalar_t)0.02, std::min((nmath::scalar_t)0.98, F));

    nmath::Vector3f wi;
    nmath::scalar_t sampled_ior;
    if (nmath::prng_c(0.0, 1.0) < p_reflect) {
        wi = wo.reflected(h).normalized();
        if ((nmath::dot(ng, wo) * nmath::dot(ng, wi)) <= (nmath::scalar_t)0.0) return false;
        sampled_ior = eta_i;
    } else {
        wi = (-wo).refracted(h, eta_i, eta_t).normalized();
        if (wi.length() <= (nmath::scalar_t)EPSILON) {
            // TIR fallback
            wi = wo.reflected(h).normalized();
            if ((nmath::dot(ng, wo) * nmath::dot(ng, wi)) <= (nmath::scalar_t)0.0) return false;
            sampled_ior = eta_i;
        } else {
            if ((nmath::dot(ng, wo) * nmath::dot(ng, wi)) > (nmath::scalar_t)0.0) return false;
            sampled_ior = eta_t;
        }
    }

    // Compute correct f*cos/pdf via eval — avoids manually re-deriving GGX geometry terms.
    nimg::ColorRGBf f;
    nmath::scalar_t pdf = 0.0;
    if (!rough_dielectric_eval_impl(this, hit_record, wo, wi, f, pdf)) return false;
    if (pdf <= (nmath::scalar_t)EPSILON) return false;

    const nmath::scalar_t cos_i = nmath_abs(nmath::dot(n, wi));
    hit_result.ray.origin = hit_record.point + wi * EPSILON;
    hit_result.ray.direction = wi;
    hit_result.ior = sampled_ior;
    hit_result.intensity = f * (cos_i / pdf);
    return true;
}

bool RoughDielectric::bsdf_is_delta() const
{
    return false;
}

bool RoughDielectric::bsdf_eval(
            const hit_record_t &hit_record
    , const Vector3f &wo
    , const Vector3f &wi
    , ColorRGBf &f
    , scalar_t &pdf
) const
{
    return rough_dielectric_eval_impl(this, hit_record, wo, wi, f, pdf);
}

bool RoughDielectric::bsdf_sample(
            const hit_record_t &hit_record
    , const Vector3f &wo
    , Vector3f &wi
    , ColorRGBf &f
    , scalar_t &pdf
) const
{
    const nmath::Vector3f ng = hit_record.normal.normalized();
    nmath::Vector3f n = shading_normal(this, hit_record);
    const nmath::Vector3f wo_n = wo.normalized();
    if (nmath::dot(n, wo_n) < (nmath::scalar_t)0.0) n = -n;

    const nmath::scalar_t roughness = material_roughness(this, hit_record);
    nmath::scalar_t eta_i = 1.0;
    nmath::scalar_t eta_t = material_ior(this);
    dielectric_media(hit_record, this, wo_n, eta_i, eta_t);

    if (use_pragmatic_frosted_lobes(roughness)) {
        const nmath::scalar_t exponent = lobe_exponent_from_roughness(roughness);
        const nmath::scalar_t F = xtcore::math::sampling::fresnel_dielectric(
            std::max((nmath::scalar_t)0.0, nmath_abs(nmath::dot(n, wo_n))),
            eta_i,
            eta_t);
        if (nmath::prng_c(0.0, 1.0) < F) {
            nmath::scalar_t tmp_pdf = 0.0;
            wi = xtcore::math::sampling::sample_power_cosine_lobe(wo_n.reflected(n).normalized(), exponent, tmp_pdf).normalized();
            if ((nmath::dot(ng, wo_n) * nmath::dot(ng, wi)) <= (nmath::scalar_t)0.0) return false;
        } else {
            nmath::Vector3f trans_axis = (-wo_n).refracted(n, eta_i, eta_t).normalized();
            if (trans_axis.length() <= (nmath::scalar_t)EPSILON) trans_axis = wo_n.reflected(n).normalized();
            nmath::scalar_t tmp_pdf = 0.0;
            wi = xtcore::math::sampling::sample_power_cosine_lobe(trans_axis, exponent, tmp_pdf).normalized();
            if ((nmath::dot(ng, wo_n) * nmath::dot(ng, wi)) > (nmath::scalar_t)0.0) return false;
        }
        return rough_dielectric_eval_impl(this, hit_record, wo_n, wi, f, pdf);
    }

    nmath::scalar_t h_pdf = 0.0;
    nmath::Vector3f h = xtcore::math::sampling::sample_ggx_half_vector(n, roughness, h_pdf);
    if (nmath::dot(wo_n, h) < (nmath::scalar_t)0.0) h = -h;

    const nmath::scalar_t F = xtcore::math::sampling::fresnel_dielectric(nmath_abs(nmath::dot(wo_n, h)), eta_i, eta_t);
    if (nmath::prng_c(0.0, 1.0) < F) {
        wi = wo_n.reflected(h).normalized();
        if ((nmath::dot(ng, wo_n) * nmath::dot(ng, wi)) <= (nmath::scalar_t)0.0) return false;
    } else {
        wi = (-wo_n).refracted(h, eta_i, eta_t).normalized();
        if (wi.length() <= (nmath::scalar_t)EPSILON) {
            wi = wo_n.reflected(h).normalized();
        }
        if ((nmath::dot(ng, wo_n) * nmath::dot(ng, wi)) > (nmath::scalar_t)0.0) return false;
    }

    return rough_dielectric_eval_impl(this, hit_record, wo_n, wi, f, pdf);
}

        } /* namespace material */
    } /* namespace asset */
} /* namespace xtcore */
