#include <algorithm>

#include <nimg/luminance.h>
#include <nmath/prng.h>
#include <xtcore/math/sampling_util.h>

#include "macro.h"
#include "thin_dielectric.h"

namespace xtcore {
    namespace asset {
        namespace material {

namespace {

inline nmath::scalar_t clamp_scalar(nmath::scalar_t v, nmath::scalar_t lo, nmath::scalar_t hi)
{
    return std::max(lo, std::min(v, hi));
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

inline nmath::scalar_t material_roughness(const xtcore::asset::IMaterial *mat, const hit_record_t &hit_record)
{
    const nmath::scalar_t scalar_value = (nmath::scalar_t)mat->get_scalar(MAT_SCALART_ROUGHNESS);
    return clamp_scalar(sample_scalar_sampler(mat, hit_record, MAT_SAMPLER_ROUGHNESS, scalar_value), (nmath::scalar_t)0.0, (nmath::scalar_t)1.0);
}

inline nmath::scalar_t material_ior(const xtcore::asset::IMaterial *mat)
{
    const nmath::scalar_t ior = (nmath::scalar_t)mat->get_scalar(MAT_SCALART_IOR);
    return (ior > (nmath::scalar_t)1.0) ? ior : (nmath::scalar_t)1.5;
}

inline nmath::scalar_t lobe_exponent_from_roughness(nmath::scalar_t roughness)
{
    const nmath::scalar_t r = clamp_scalar(roughness, (nmath::scalar_t)0.02, (nmath::scalar_t)1.0);
    return std::max((nmath::scalar_t)2.0, ((nmath::scalar_t)2.0 / (r * r)) - (nmath::scalar_t)2.0);
}

inline nmath::scalar_t safe_luma(const nimg::ColorRGBf &c)
{
    return (nmath::scalar_t)nimg::eval::luminance(c);
}

inline bool thin_dielectric_is_delta_material(const xtcore::asset::IMaterial *mat)
{
    if (!mat) return true;
    if (mat->has_sampler(MAT_SAMPLER_ROUGHNESS)) return false;
    return (nmath::scalar_t)mat->get_scalar(MAT_SCALART_ROUGHNESS) <= (nmath::scalar_t)0.02;
}

inline bool eval_thin_dielectric(const xtcore::asset::IMaterial *mat,
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

    const nmath::scalar_t cos_o = nmath_abs(nmath::dot(ng, wo_n));
    const nmath::scalar_t cos_i = nmath_abs(nmath::dot(ng, wi_n));
    if (cos_o <= (nmath::scalar_t)EPSILON || cos_i <= (nmath::scalar_t)EPSILON) {
        f = nimg::ColorRGBf(0.0f, 0.0f, 0.0f);
        pdf = 0.0f;
        return false;
    }

    const nmath::scalar_t ior = material_ior(mat);
    const nmath::scalar_t roughness = material_roughness(mat, hit_record);
    const nmath::scalar_t exponent = lobe_exponent_from_roughness(roughness);
    const nmath::scalar_t transparency = clamp_scalar((nmath::scalar_t)mat->get_scalar(MAT_SCALART_TRANSPARENCY), (nmath::scalar_t)0.0, (nmath::scalar_t)1.0);
    const nimg::ColorRGBf trans = transmission_color(mat, hit_record) * transparency;
    const nmath::scalar_t F = xtcore::math::sampling::fresnel_dielectric(
        std::max((nmath::scalar_t)0.0, nmath::dot(n, wo_n)),
        (nmath::scalar_t)1.0,
        ior);
    const nmath::scalar_t p_reflect = std::max((nmath::scalar_t)0.02, std::min((nmath::scalar_t)0.98, F));
    const nmath::scalar_t p_transmit = std::max((nmath::scalar_t)EPSILON, (nmath::scalar_t)1.0 - p_reflect);

    const bool same_hemi = (nmath::dot(ng, wo_n) * nmath::dot(ng, wi_n)) > (nmath::scalar_t)0.0;
    const nmath::Vector3f reflect_axis = wo_n.reflected(n).normalized();
    const nmath::Vector3f transmit_axis = (-wo_n).normalized();
    const nmath::scalar_t pdf_reflect = xtcore::math::sampling::power_cosine_lobe_pdf(reflect_axis, wi_n, exponent);
    const nmath::scalar_t pdf_transmit = xtcore::math::sampling::power_cosine_lobe_pdf(transmit_axis, wi_n, exponent);

    const nmath::scalar_t brdf_pdf_ratio = (exponent + (nmath::scalar_t)2.0) / (exponent + (nmath::scalar_t)1.0);

    if (same_hemi) {
        pdf = p_reflect * pdf_reflect;
        if (pdf <= (nmath::scalar_t)EPSILON) {
            f = nimg::ColorRGBf(0.0f, 0.0f, 0.0f);
            return false;
        }
        const nmath::scalar_t brdf_val = F * brdf_pdf_ratio * pdf_reflect;
        f = nimg::ColorRGBf((float)brdf_val, (float)brdf_val, (float)brdf_val);
        return safe_luma(f) > (nmath::scalar_t)EPSILON;
    }

    pdf = p_transmit * pdf_transmit;
    if (pdf <= (nmath::scalar_t)EPSILON) {
        f = nimg::ColorRGBf(0.0f, 0.0f, 0.0f);
        return false;
    }
    f = trans * (((nmath::scalar_t)1.0 - F) * brdf_pdf_ratio * pdf_transmit);
    return safe_luma(f) > (nmath::scalar_t)EPSILON;
}

} // namespace

bool ThinDielectric::shade(
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

bool ThinDielectric::sample_path(
            hit_result_t &hit_result
    , const hit_record_t &hit_record
) const
{
    if (!bsdf_is_delta()) {
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

    const nmath::Vector3f in_dir = hit_record.incident_direction.normalized();
    nmath::Vector3f n = shading_normal(this, hit_record);
    if (nmath::dot(n, -in_dir) < (nmath::scalar_t)0.0) n = -n;

    const nmath::scalar_t ior = material_ior(this);
    const nmath::scalar_t roughness = material_roughness(this, hit_record);
    const nmath::scalar_t transparency = clamp_scalar((nmath::scalar_t)get_scalar(MAT_SCALART_TRANSPARENCY), (nmath::scalar_t)0.0, (nmath::scalar_t)1.0);
    const nimg::ColorRGBf trans = transmission_color(this, hit_record) * transparency;

    const nmath::scalar_t F = xtcore::math::sampling::fresnel_dielectric(std::max((nmath::scalar_t)0.0, nmath::dot(n, -in_dir)), (nmath::scalar_t)1.0, ior);
    const nmath::scalar_t p_reflect = std::max((nmath::scalar_t)0.02, std::min((nmath::scalar_t)0.98, F));

    nmath::Vector3f out_dir;
    if (nmath::prng_c(0.0, 1.0) < p_reflect) {
        out_dir = in_dir.reflected(n).normalized();
        if (roughness > (nmath::scalar_t)0.02) {
            nmath::scalar_t pdf = 0.0;
            out_dir = xtcore::math::sampling::sample_power_cosine_lobe(out_dir, lobe_exponent_from_roughness(roughness), pdf);
        }
        hit_result.intensity = nimg::ColorRGBf((float)(F / p_reflect), (float)(F / p_reflect), (float)(F / p_reflect));
    } else {
        out_dir = in_dir;
        if (roughness > (nmath::scalar_t)0.02) {
            nmath::scalar_t pdf = 0.0;
            out_dir = xtcore::math::sampling::sample_power_cosine_lobe(out_dir, lobe_exponent_from_roughness(roughness), pdf);
        }
        const nmath::scalar_t p_transmit = std::max((nmath::scalar_t)EPSILON, (nmath::scalar_t)1.0 - p_reflect);
        hit_result.intensity = trans * (((nmath::scalar_t)1.0 - F) / p_transmit);
    }

    hit_result.ray.origin = hit_record.point + out_dir * EPSILON;
    hit_result.ray.direction = out_dir.normalized();
    hit_result.ior = hit_record.ior > (nmath::scalar_t)EPSILON ? hit_record.ior : (nmath::scalar_t)1.0;
    return true;
}

bool ThinDielectric::bsdf_is_delta() const
{
    return thin_dielectric_is_delta_material(this);
}

bool ThinDielectric::bsdf_eval(
            const hit_record_t &hit_record
    , const Vector3f &wo
    , const Vector3f &wi
    , ColorRGBf &f
    , scalar_t &pdf
) const
{
    return eval_thin_dielectric(this, hit_record, wo, wi, f, pdf);
}

bool ThinDielectric::bsdf_sample(
            const hit_record_t &hit_record
    , const Vector3f &wo
    , Vector3f &wi
    , ColorRGBf &f
    , scalar_t &pdf
) const
{
    nmath::Vector3f n = shading_normal(this, hit_record);
    const nmath::Vector3f wo_n = wo.normalized();
    if (nmath::dot(n, wo_n) < (nmath::scalar_t)0.0) n = -n;
    const nmath::scalar_t ior = material_ior(this);
    const nmath::scalar_t roughness = material_roughness(this, hit_record);
    const nmath::scalar_t exponent = lobe_exponent_from_roughness(roughness);
    const nmath::scalar_t F = xtcore::math::sampling::fresnel_dielectric(
        std::max((nmath::scalar_t)0.0, nmath::dot(n, wo_n)),
        (nmath::scalar_t)1.0,
        ior);

    nmath::scalar_t tmp_pdf = 0.0;
    if (nmath::prng_c(0.0, 1.0) < F) {
        const nmath::Vector3f reflect_axis = wo_n.reflected(n).normalized();
        wi = xtcore::math::sampling::sample_power_cosine_lobe(reflect_axis, exponent, tmp_pdf);
    } else {
        const nmath::Vector3f transmit_axis = (-wo_n).normalized();
        wi = xtcore::math::sampling::sample_power_cosine_lobe(transmit_axis, exponent, tmp_pdf);
    }
    return eval_thin_dielectric(this, hit_record, wo_n, wi, f, pdf);
}

        } /* namespace material */
    } /* namespace asset */
} /* namespace xtcore */
