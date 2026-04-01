#include <algorithm>

#include <nimg/luminance.h>
#include <nmath/prng.h>
#include <xtcore/math/sampling_util.h>

#include "macro.h"
#include "principled.h"

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

inline nimg::ColorRGBf sample_base_color(const xtcore::asset::IMaterial *mat, const hit_record_t &hit_record)
{
    if (mat->has_sampler(MAT_SAMPLER_BASE_COLOR)) return mat->get_sample(MAT_SAMPLER_BASE_COLOR, hit_record.texcoord);
    if (mat->has_sampler(MAT_SAMPLER_DIFFUSE)) return mat->get_sample(MAT_SAMPLER_DIFFUSE, hit_record.texcoord);
    return nimg::ColorRGBf(1.0f, 1.0f, 1.0f);
}

inline nmath::scalar_t material_roughness(const xtcore::asset::IMaterial *mat, const hit_record_t &hit_record)
{
    const nmath::scalar_t scalar_value = (nmath::scalar_t)mat->get_scalar(MAT_SCALART_ROUGHNESS);
    return clamp_scalar(sample_scalar_sampler(mat, hit_record, MAT_SAMPLER_ROUGHNESS, scalar_value), (nmath::scalar_t)0.02, (nmath::scalar_t)1.0);
}

inline nmath::scalar_t material_metallic(const xtcore::asset::IMaterial *mat, const hit_record_t &hit_record)
{
    const nmath::scalar_t scalar_value = (nmath::scalar_t)mat->get_scalar(MAT_SCALART_METALLIC);
    return clamp_scalar(sample_scalar_sampler(mat, hit_record, MAT_SAMPLER_METALLIC, scalar_value), (nmath::scalar_t)0.0, (nmath::scalar_t)1.0);
}

inline nmath::scalar_t material_ior(const xtcore::asset::IMaterial *mat)
{
    const nmath::scalar_t ior = (nmath::scalar_t)mat->get_scalar(MAT_SCALART_IOR);
    return (ior > (nmath::scalar_t)1.0) ? ior : (nmath::scalar_t)1.5;
}

inline nmath::scalar_t material_clearcoat(const xtcore::asset::IMaterial *mat)
{
    return clamp_scalar((nmath::scalar_t)mat->get_scalar(MAT_SCALART_CLEARCOAT), (nmath::scalar_t)0.0, (nmath::scalar_t)1.0);
}

inline nmath::scalar_t material_clearcoat_roughness(const xtcore::asset::IMaterial *mat)
{
    const nmath::scalar_t value = (nmath::scalar_t)mat->get_scalar(MAT_SCALART_CLEARCOAT_ROUGHNESS);
    const nmath::scalar_t fallback = value > (nmath::scalar_t)EPSILON ? value : (nmath::scalar_t)0.08;
    return clamp_scalar(fallback, (nmath::scalar_t)0.02, (nmath::scalar_t)0.6);
}

struct clearcoat_layer_t
{
    nmath::scalar_t amount;
    nmath::scalar_t roughness;
};

struct principled_sampling_weights_t
{
    nmath::scalar_t diffuse;
    nmath::scalar_t specular;
    nmath::scalar_t clearcoat;
};

inline clearcoat_layer_t material_clearcoat_layer(const xtcore::asset::IMaterial *mat)
{
    const nmath::scalar_t clearcoat = material_clearcoat(mat);
    const nmath::scalar_t clearcoat_roughness = material_clearcoat_roughness(mat);

    clearcoat_layer_t layer;
    layer.amount = clearcoat * (nmath::scalar_t)0.6;
    layer.roughness = std::max((nmath::scalar_t)0.02, clearcoat_roughness * clearcoat_roughness);
    return layer;
}

inline principled_sampling_weights_t principled_sampling_weights(const nimg::ColorRGBf &kd,
                                                                 const nimg::ColorRGBf &f0,
                                                                 nmath::scalar_t clearcoat_amount)
{
    principled_sampling_weights_t weights;

    const nmath::scalar_t base_weight_scale = (nmath::scalar_t)1.0 - ((nmath::scalar_t)0.35 * clearcoat_amount);
    weights.diffuse = std::max((nmath::scalar_t)0.02, safe_luma(kd) * base_weight_scale);
    weights.specular = std::max((nmath::scalar_t)0.04, safe_luma(f0) * base_weight_scale);
    weights.clearcoat = clearcoat_amount > (nmath::scalar_t)EPSILON
        ? std::max((nmath::scalar_t)0.12, clearcoat_amount)
        : (nmath::scalar_t)0.0;
    return weights;
}

inline nmath::scalar_t material_anisotropy(const xtcore::asset::IMaterial *mat)
{
    return clamp_scalar((nmath::scalar_t)mat->get_scalar(MAT_SCALART_ANISOTROPY), (nmath::scalar_t)0.0, (nmath::scalar_t)1.0);
}

inline nmath::scalar_t material_anisotropy_rotation(const xtcore::asset::IMaterial *mat)
{
    return (nmath::scalar_t)mat->get_scalar(MAT_SCALART_ANISOTROPY_ROTATION);
}

inline nmath::scalar_t dielectric_f0_scalar(nmath::scalar_t ior)
{
    const nmath::scalar_t num = ior - (nmath::scalar_t)1.0;
    const nmath::scalar_t den = ior + (nmath::scalar_t)1.0;
    const nmath::scalar_t r = num / std::max((nmath::scalar_t)EPSILON, den);
    return r * r;
}

inline nmath::scalar_t fresnel_schlick_scalar(nmath::scalar_t f0, nmath::scalar_t cos_theta)
{
    const nmath::scalar_t ct = clamp_scalar(cos_theta, (nmath::scalar_t)0.0, (nmath::scalar_t)1.0);
    const nmath::scalar_t m = (nmath::scalar_t)1.0 - ct;
    const nmath::scalar_t m2 = m * m;
    const nmath::scalar_t m5 = m2 * m2 * m;
    return f0 + (((nmath::scalar_t)1.0 - f0) * m5);
}

inline void principled_lobes(const xtcore::asset::IMaterial *mat,
                             const hit_record_t &hit_record,
                             nimg::ColorRGBf &kd,
                             nimg::ColorRGBf &f0,
                             nmath::scalar_t &roughness,
                             nmath::scalar_t &anisotropy,
                             nmath::scalar_t &anisotropy_rotation)
{
    const nimg::ColorRGBf base_color = sample_base_color(mat, hit_record);
    const nmath::scalar_t metallic = material_metallic(mat, hit_record);
    roughness = material_roughness(mat, hit_record);
    anisotropy = material_anisotropy(mat);
    anisotropy_rotation = material_anisotropy_rotation(mat);

    const nmath::scalar_t f0_dielectric = dielectric_f0_scalar(material_ior(mat));
    const nimg::ColorRGBf dielectric_f0((float)f0_dielectric, (float)f0_dielectric, (float)f0_dielectric);

    kd = base_color * ((nmath::scalar_t)1.0 - metallic);
    f0 = dielectric_f0 * ((nmath::scalar_t)1.0 - metallic) + base_color * metallic;
}

inline void anisotropic_basis(const xtcore::asset::IMaterial *mat,
                              const hit_record_t &hit_record,
                              const nmath::Vector3f &n,
                              nmath::Vector3f &t,
                              nmath::Vector3f &b)
{
    t = xtcore::math::sampling::build_tangent(n);
    b = nmath::cross(n, t).normalized();

    const nmath::scalar_t rotation_deg = material_anisotropy_rotation(mat);
    if (nmath_abs(rotation_deg) <= (nmath::scalar_t)EPSILON) return;

    const nmath::scalar_t radians = rotation_deg * ((nmath::scalar_t)nmath::PI / (nmath::scalar_t)180.0);
    const nmath::scalar_t cs = nmath_cos(radians);
    const nmath::scalar_t sn = nmath_sin(radians);
    const nmath::Vector3f rt = (t * cs) + (b * sn);
    const nmath::Vector3f rb = (b * cs) - (t * sn);
    t = rt.normalized();
    b = rb.normalized();
    (void)hit_record;
}

inline nmath::scalar_t specular_pdf(const nmath::Vector3f &n,
                                    const nmath::Vector3f &t,
                                    const nmath::Vector3f &b,
                                    const nmath::Vector3f &wo,
                                    const nmath::Vector3f &wi,
                                    nmath::scalar_t roughness,
                                    nmath::scalar_t anisotropy)
{
    const nmath::Vector3f h = (wo + wi).normalized();
    const nmath::scalar_t voh = std::max((nmath::scalar_t)EPSILON, nmath_abs(nmath::dot(wo, h)));
    const nmath::scalar_t aspect = nmath_sqrt(std::max((nmath::scalar_t)0.1, (nmath::scalar_t)1.0 - ((nmath::scalar_t)0.9 * anisotropy)));
    const nmath::scalar_t ax = std::max((nmath::scalar_t)0.02, roughness / aspect);
    const nmath::scalar_t ay = std::max((nmath::scalar_t)0.02, roughness * aspect);
    const nmath::scalar_t d = xtcore::math::sampling::ggx_ndf_anisotropic(n, t, b, h, ax, ay);
    const nmath::scalar_t nh = std::max((nmath::scalar_t)0.0, nmath::dot(n, h));
    return (d * nh) / ((nmath::scalar_t)4.0 * voh);
}

inline bool eval_principled(const xtcore::asset::IMaterial *mat,
                            const hit_record_t &hit_record,
                            const nmath::Vector3f &wo,
                            const nmath::Vector3f &wi,
                            nimg::ColorRGBf &f,
                            nmath::scalar_t &pdf)
{
    const nmath::Vector3f n = shading_normal(mat, hit_record);
    const nmath::Vector3f wo_n = wo.normalized();
    const nmath::Vector3f wi_n = wi.normalized();
    const nmath::scalar_t cos_i = std::max((nmath::scalar_t)0.0, nmath::dot(n, wi_n));
    const nmath::scalar_t cos_o = std::max((nmath::scalar_t)0.0, nmath::dot(n, wo_n));
    if (cos_i <= (nmath::scalar_t)EPSILON || cos_o <= (nmath::scalar_t)EPSILON) {
        f = nimg::ColorRGBf(0.0f, 0.0f, 0.0f);
        pdf = 0.0f;
        return false;
    }

    nimg::ColorRGBf kd, f0;
    nmath::scalar_t roughness = 0.5;
    nmath::scalar_t anisotropy = 0.0;
    nmath::scalar_t anisotropy_rotation = 0.0;
    principled_lobes(mat, hit_record, kd, f0, roughness, anisotropy, anisotropy_rotation);
    nmath::Vector3f t;
    nmath::Vector3f b;
    anisotropic_basis(mat, hit_record, n, t, b);
    const nmath::scalar_t aspect = nmath_sqrt(std::max((nmath::scalar_t)0.1, (nmath::scalar_t)1.0 - ((nmath::scalar_t)0.9 * anisotropy)));
    const nmath::scalar_t ax = std::max((nmath::scalar_t)0.02, roughness / aspect);
    const nmath::scalar_t ay = std::max((nmath::scalar_t)0.02, roughness * aspect);

    const nmath::Vector3f h = (wo_n + wi_n).normalized();
    if (h.length() <= (nmath::scalar_t)EPSILON) {
        f = nimg::ColorRGBf(0.0f, 0.0f, 0.0f);
        pdf = 0.0f;
        return false;
    }

    const nmath::scalar_t d = xtcore::math::sampling::ggx_ndf_anisotropic(n, t, b, h, ax, ay);
    const nmath::scalar_t g = xtcore::math::sampling::smith_ggx_g_anisotropic(n, t, b, wo_n, wi_n, ax, ay);
    const nimg::ColorRGBf  fresnel = xtcore::math::sampling::fresnel_schlick(f0, std::max((nmath::scalar_t)0.0, nmath::dot(h, wi_n)));

    const clearcoat_layer_t clearcoat = material_clearcoat_layer(mat);
    const nmath::scalar_t spec_denom = std::max((nmath::scalar_t)EPSILON, (nmath::scalar_t)4.0 * cos_i * cos_o);
    const nimg::ColorRGBf specular = fresnel * (d * g / spec_denom);
    const nimg::ColorRGBf clearcoat_f0(0.04f, 0.04f, 0.04f);
    const nmath::scalar_t coat_fo = fresnel_schlick_scalar((nmath::scalar_t)0.04, cos_o);
    const nmath::scalar_t coat_fi = fresnel_schlick_scalar((nmath::scalar_t)0.04, cos_i);
    const nmath::scalar_t base_attenuation = ((nmath::scalar_t)1.0 - clearcoat.amount * coat_fo)
                                           * ((nmath::scalar_t)1.0 - clearcoat.amount * coat_fi);
    const nimg::ColorRGBf clearcoat_fresnel = xtcore::math::sampling::fresnel_schlick(
        clearcoat_f0,
        std::max((nmath::scalar_t)0.0, nmath::dot(h, wi_n)));
    const nmath::scalar_t clearcoat_d = xtcore::math::sampling::ggx_ndf(n, h, clearcoat.roughness);
    const nmath::scalar_t clearcoat_g = xtcore::math::sampling::smith_ggx_g(n, wo_n, wi_n, clearcoat.roughness);
    const nimg::ColorRGBf clearcoat_specular = clearcoat_fresnel * (clearcoat.amount * clearcoat_d * clearcoat_g / spec_denom);
    const nimg::ColorRGBf diffuse = kd * (base_attenuation * ((nmath::scalar_t)1.0 / nmath::PI));
    const nimg::ColorRGBf base_specular = specular * base_attenuation;

    const principled_sampling_weights_t weights = principled_sampling_weights(kd, f0, clearcoat.amount);
    const nmath::scalar_t diffuse_weight = weights.diffuse;
    const nmath::scalar_t specular_weight = weights.specular;
    const nmath::scalar_t clearcoat_weight = weights.clearcoat;
    const nmath::scalar_t weight_sum = std::max((nmath::scalar_t)EPSILON, diffuse_weight + specular_weight + clearcoat_weight);
    const nmath::scalar_t p_diff = diffuse_weight / weight_sum;
    const nmath::scalar_t p_spec = specular_weight / weight_sum;
    const nmath::scalar_t p_clearcoat = clearcoat_weight / weight_sum;

    f = diffuse + base_specular + clearcoat_specular;
    pdf = p_diff * (cos_i / nmath::PI)
        + p_spec * specular_pdf(n, t, b, wo_n, wi_n, roughness, anisotropy)
        + p_clearcoat * specular_pdf(n, t, b, wo_n, wi_n, clearcoat.roughness, (nmath::scalar_t)0.0);
    return (pdf > (nmath::scalar_t)EPSILON) && (safe_luma(f) > (nmath::scalar_t)EPSILON);
}

} // namespace

bool Principled::shade(
            ColorRGBf    &intensity
    , const ICamera      *camera
    , const emitter_t    *emitter
    , const hit_record_t &hit_record) const
{
    UNUSED(camera)

    const nmath::Vector3f to_light = (emitter->position - hit_record.point).normalized();
    const nmath::Vector3f wo = (-hit_record.incident_direction).normalized();

    nimg::ColorRGBf f;
    nmath::scalar_t pdf = 0.0;
    if (!eval_principled(this, hit_record, wo, to_light, f, pdf)) return true;

    const nmath::scalar_t cos_i = std::max((nmath::scalar_t)0.0, nmath::dot(shading_normal(this, hit_record), to_light));
    intensity += emitter->intensity * f * cos_i;
    return true;
}

bool Principled::sample_path(
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
    const nmath::scalar_t cos_theta = std::max((nmath::scalar_t)0.0, nmath::dot(ng, wi));
    if (cos_theta <= (nmath::scalar_t)EPSILON || pdf <= (nmath::scalar_t)EPSILON) return false;

    hit_result.ray.origin = hit_record.point + ng * EPSILON;
    hit_result.ray.direction = wi;
    hit_result.intensity = f * (cos_theta / pdf);
    hit_result.ior = hit_record.ior > (nmath::scalar_t)EPSILON ? hit_record.ior : (nmath::scalar_t)1.0;
    return true;
}

bool Principled::bsdf_eval(
            const hit_record_t &hit_record
    , const Vector3f &wo
    , const Vector3f &wi
    , ColorRGBf &f
    , scalar_t &pdf
) const
{
    return eval_principled(this, hit_record, wo, wi, f, pdf);
}

bool Principled::bsdf_sample(
            const hit_record_t &hit_record
    , const Vector3f &wo
    , Vector3f &wi
    , ColorRGBf &f
    , scalar_t &pdf
) const
{
    nimg::ColorRGBf kd, f0;
    nmath::scalar_t roughness = 0.5;
    nmath::scalar_t anisotropy = 0.0;
    nmath::scalar_t anisotropy_rotation = 0.0;
    principled_lobes(this, hit_record, kd, f0, roughness, anisotropy, anisotropy_rotation);
    const clearcoat_layer_t clearcoat = material_clearcoat_layer(this);
    const nmath::Vector3f n = shading_normal(this, hit_record);
    nmath::Vector3f t;
    nmath::Vector3f b;
    anisotropic_basis(this, hit_record, n, t, b);
    const nmath::Vector3f wo_n = wo.normalized();
    const nmath::scalar_t aspect = nmath_sqrt(std::max((nmath::scalar_t)0.1, (nmath::scalar_t)1.0 - ((nmath::scalar_t)0.9 * anisotropy)));
    const nmath::scalar_t ax = std::max((nmath::scalar_t)0.02, roughness / aspect);
    const nmath::scalar_t ay = std::max((nmath::scalar_t)0.02, roughness * aspect);
    const principled_sampling_weights_t weights = principled_sampling_weights(kd, f0, clearcoat.amount);
    const nmath::scalar_t diffuse_weight = weights.diffuse;
    const nmath::scalar_t specular_weight = weights.specular;
    const nmath::scalar_t clearcoat_weight = weights.clearcoat;
    const nmath::scalar_t weight_sum = std::max((nmath::scalar_t)EPSILON, diffuse_weight + specular_weight + clearcoat_weight);
    const nmath::scalar_t choose_spec = specular_weight / weight_sum;
    const nmath::scalar_t choose_clearcoat = clearcoat_weight / weight_sum;

    const nmath::scalar_t u = nmath::prng_c(0.0, 1.0);
    if (u < choose_spec) {
        nmath::scalar_t h_pdf = 0.0;
        const nmath::Vector3f h = xtcore::math::sampling::sample_ggx_half_vector_anisotropic(n, t, b, ax, ay, h_pdf);
        wi = wo_n.reflected(h).normalized();
        if (nmath::dot(n, wi) <= (nmath::scalar_t)EPSILON) return false;
    } else if (u < choose_spec + choose_clearcoat) {
        nmath::scalar_t h_pdf = 0.0;
        const nmath::Vector3f h = xtcore::math::sampling::sample_ggx_half_vector(n, clearcoat.roughness, h_pdf);
        wi = wo_n.reflected(h).normalized();
        if (nmath::dot(n, wi) <= (nmath::scalar_t)EPSILON) return false;
    } else {
        nmath::scalar_t tmp_pdf = 0.0;
        wi = xtcore::math::sampling::sample_cosine_hemisphere(n, tmp_pdf);
    }

    return bsdf_eval(hit_record, wo, wi, f, pdf);
}

        } /* namespace material */
    } /* namespace asset */
} /* namespace xtcore */
