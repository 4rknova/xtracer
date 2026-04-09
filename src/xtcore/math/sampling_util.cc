#include <algorithm>

#include "sampling_util.h"

namespace xtcore {
namespace math {
namespace sampling {

namespace {

inline nmath::scalar_t clamp_scalar(nmath::scalar_t v, nmath::scalar_t lo, nmath::scalar_t hi)
{
    return std::max(lo, std::min(v, hi));
}

inline nmath::scalar_t clamp_roughness(nmath::scalar_t roughness)
{
    return clamp_scalar(roughness, (nmath::scalar_t)0.02, (nmath::scalar_t)1.0);
}

inline nmath::scalar_t clamp_alpha(nmath::scalar_t alpha)
{
    return clamp_scalar(alpha, (nmath::scalar_t)0.02, (nmath::scalar_t)1.0);
}

} // namespace

nmath::Vector3f build_tangent(const nmath::Vector3f &n)
{
    const nmath::Vector3f up = (nmath_abs(n.z) < 0.999f)
                             ? nmath::Vector3f(0.0f, 0.0f, 1.0f)
                             : nmath::Vector3f(1.0f, 0.0f, 0.0f);
    nmath::Vector3f t = nmath::cross(up, n);
    if (t.length() <= (nmath::scalar_t)EPSILON) {
        t = nmath::Vector3f(1.0f, 0.0f, 0.0f);
    } else {
        t.normalize();
    }
    return t;
}

nmath::Vector3f sample_uniform_sphere(nmath::scalar_t &out_pdf)
{
    const nmath::scalar_t u1 = nmath::prng_c(0.0, 1.0);
    const nmath::scalar_t u2 = nmath::prng_c(0.0, 1.0);
    const nmath::scalar_t z = (nmath::scalar_t)1.0 - (nmath::scalar_t)2.0 * u1;
    const nmath::scalar_t r = nmath_sqrt(std::max((nmath::scalar_t)0.0, (nmath::scalar_t)1.0 - z * z));
    const nmath::scalar_t phi = ((nmath::scalar_t)2.0 * nmath::PI) * u2;

    nmath::Vector3f dir(r * nmath_cos(phi), z, r * nmath_sin(phi));
    dir.normalize();
    out_pdf = uniform_sphere_pdf();
    return dir;
}

nmath::scalar_t uniform_sphere_pdf()
{
    return (nmath::scalar_t)1.0 / ((nmath::scalar_t)4.0 * nmath::PI);
}

nmath::scalar_t uniform_cone_pdf(nmath::scalar_t cos_theta_max)
{
    const nmath::scalar_t solid_angle = (nmath::scalar_t)2.0 * nmath::PI * ((nmath::scalar_t)1.0 - cos_theta_max);
    return (solid_angle > (nmath::scalar_t)EPSILON) ? ((nmath::scalar_t)1.0 / solid_angle) : (nmath::scalar_t)0.0;
}

nmath::Vector3f sample_uniform_cone(const nmath::Vector3f &axis,
                                    nmath::scalar_t cos_theta_max,
                                    nmath::scalar_t &out_pdf)
{
    const nmath::scalar_t u1 = nmath::prng_c(0.0, 1.0);
    const nmath::scalar_t u2 = nmath::prng_c(0.0, 1.0);

    const nmath::scalar_t cos_theta = (nmath::scalar_t)1.0 - u1 * ((nmath::scalar_t)1.0 - cos_theta_max);
    const nmath::scalar_t sin_theta = nmath_sqrt(std::max((nmath::scalar_t)0.0, (nmath::scalar_t)1.0 - cos_theta * cos_theta));
    const nmath::scalar_t phi = ((nmath::scalar_t)2.0 * nmath::PI) * u2;

    const nmath::Vector3f n = axis.normalized();
    const nmath::Vector3f t = build_tangent(n);
    const nmath::Vector3f b = nmath::cross(n, t).normalized();

    nmath::Vector3f dir = (t * (sin_theta * nmath_cos(phi)))
                        + (n * cos_theta)
                        + (b * (sin_theta * nmath_sin(phi)));
    dir.normalize();

    out_pdf = uniform_cone_pdf(cos_theta_max);
    return dir;
}

nmath::Vector3f sample_cosine_hemisphere(const nmath::Vector3f &normal, nmath::scalar_t &out_pdf)
{
    const nmath::scalar_t u1 = nmath::prng_c(0.0, 1.0);
    const nmath::scalar_t u2 = nmath::prng_c(0.0, 1.0);
    const nmath::scalar_t r = nmath_sqrt(std::max((nmath::scalar_t)0.0, u1));
    const nmath::scalar_t phi = nmath::PI_DOUBLE * u2;

    const nmath::scalar_t x = r * nmath_cos(phi);
    const nmath::scalar_t z = r * nmath_sin(phi);
    const nmath::scalar_t y = nmath_sqrt(std::max((nmath::scalar_t)0.0, (nmath::scalar_t)(1.0 - u1)));

    const nmath::Vector3f n = normal.normalized();
    const nmath::Vector3f t = build_tangent(n);
    const nmath::Vector3f b = nmath::cross(n, t).normalized();

    nmath::Vector3f dir = (t * x) + (n * y) + (b * z);
    dir.normalize();

    const nmath::scalar_t cos_theta = std::max((nmath::scalar_t)0.0, nmath::dot(n, dir));
    out_pdf = cos_theta / nmath::PI;
    return dir;
}

nmath::Vector3f sample_power_cosine_lobe(const nmath::Vector3f &axis,
                                         nmath::scalar_t exponent,
                                         nmath::scalar_t &out_pdf)
{
    const nmath::Vector3f a = axis.normalized();
    const nmath::Vector3f t = build_tangent(a);
    const nmath::Vector3f b = nmath::cross(a, t).normalized();

    const nmath::scalar_t u1 = nmath::prng_c(0.0, 1.0);
    const nmath::scalar_t u2 = nmath::prng_c(0.0, 1.0);

    const nmath::scalar_t phi = nmath::PI_DOUBLE * u1;
    const nmath::scalar_t cos_theta = nmath_pow(u2, (nmath::scalar_t)(1.0 / (exponent + (nmath::scalar_t)1.0)));
    const nmath::scalar_t sin_theta = nmath_sqrt(std::max((nmath::scalar_t)0.0, (nmath::scalar_t)1.0 - cos_theta * cos_theta));

    nmath::Vector3f dir = (t * (sin_theta * nmath_cos(phi)))
                        + (a * cos_theta)
                        + (b * (sin_theta * nmath_sin(phi)));
    dir.normalize();

    const nmath::scalar_t ca = std::max((nmath::scalar_t)0.0, nmath::dot(a, dir));
    out_pdf = ((exponent + (nmath::scalar_t)1.0) / ((nmath::scalar_t)2.0 * nmath::PI)) * nmath_pow(ca, exponent);
    return dir;
}

nmath::scalar_t power_cosine_lobe_pdf(const nmath::Vector3f &axis,
                                      const nmath::Vector3f &dir,
                                      nmath::scalar_t exponent)
{
    const nmath::scalar_t ca = nmath::dot(axis.normalized(), dir.normalized());
    if (ca <= (nmath::scalar_t)0.0) return 0.0;
    return ((exponent + (nmath::scalar_t)1.0) / ((nmath::scalar_t)2.0 * nmath::PI)) * nmath_pow(ca, exponent);
}

nmath::scalar_t ggx_ndf(const nmath::Vector3f &normal,
                        const nmath::Vector3f &half_vector,
                        nmath::scalar_t roughness)
{
    const nmath::scalar_t alpha = clamp_roughness(roughness);
    const nmath::scalar_t a2 = alpha * alpha;
    const nmath::scalar_t nh = std::max((nmath::scalar_t)0.0, nmath::dot(normal.normalized(), half_vector.normalized()));
    if (nh <= (nmath::scalar_t)EPSILON) return 0.0;

    const nmath::scalar_t nh2 = nh * nh;
    const nmath::scalar_t denom = nh2 * (a2 - (nmath::scalar_t)1.0) + (nmath::scalar_t)1.0;
    return a2 / (nmath::PI * denom * denom);
}

nmath::scalar_t ggx_ndf_anisotropic(const nmath::Vector3f &normal,
                                    const nmath::Vector3f &tangent,
                                    const nmath::Vector3f &bitangent,
                                    const nmath::Vector3f &half_vector,
                                    nmath::scalar_t alpha_x,
                                    nmath::scalar_t alpha_y)
{
    const nmath::Vector3f n = normal.normalized();
    const nmath::Vector3f t = tangent.normalized();
    const nmath::Vector3f b = bitangent.normalized();
    const nmath::Vector3f h = half_vector.normalized();
    const nmath::scalar_t nh = std::max((nmath::scalar_t)0.0, nmath::dot(n, h));
    if (nh <= (nmath::scalar_t)EPSILON) return 0.0;

    const nmath::scalar_t ax = clamp_alpha(alpha_x);
    const nmath::scalar_t ay = clamp_alpha(alpha_y);
    const nmath::scalar_t hx = nmath::dot(h, t);
    const nmath::scalar_t hy = nmath::dot(h, b);
    const nmath::scalar_t hz = nh;
    const nmath::scalar_t sx = hx / ax;
    const nmath::scalar_t sy = hy / ay;
    const nmath::scalar_t slope = sx * sx + sy * sy + hz * hz;
    const nmath::scalar_t denom = nmath::PI * ax * ay * slope * slope;
    return ((nmath::scalar_t)1.0 / std::max((nmath::scalar_t)EPSILON, denom));
}

nmath::scalar_t smith_ggx_g1(const nmath::Vector3f &normal,
                             const nmath::Vector3f &dir,
                             nmath::scalar_t roughness)
{
    const nmath::scalar_t alpha = clamp_roughness(roughness);
    const nmath::scalar_t cos_theta = std::max((nmath::scalar_t)0.0, nmath::dot(normal.normalized(), dir.normalized()));
    if (cos_theta <= (nmath::scalar_t)EPSILON) return 0.0;

    const nmath::scalar_t sin2_theta = std::max((nmath::scalar_t)0.0, (nmath::scalar_t)1.0 - cos_theta * cos_theta);
    if (sin2_theta <= (nmath::scalar_t)EPSILON) return 1.0;

    const nmath::scalar_t tan2_theta = sin2_theta / std::max((nmath::scalar_t)EPSILON, cos_theta * cos_theta);
    const nmath::scalar_t root = nmath_sqrt((nmath::scalar_t)1.0 + alpha * alpha * tan2_theta);
    return ((nmath::scalar_t)2.0 * cos_theta) / (cos_theta + root);
}

nmath::scalar_t smith_ggx_g1_anisotropic(const nmath::Vector3f &normal,
                                         const nmath::Vector3f &tangent,
                                         const nmath::Vector3f &bitangent,
                                         const nmath::Vector3f &dir,
                                         nmath::scalar_t alpha_x,
                                         nmath::scalar_t alpha_y)
{
    const nmath::Vector3f n = normal.normalized();
    const nmath::Vector3f t = tangent.normalized();
    const nmath::Vector3f b = bitangent.normalized();
    const nmath::Vector3f v = dir.normalized();
    const nmath::scalar_t cos_theta = std::max((nmath::scalar_t)0.0, nmath::dot(n, v));
    if (cos_theta <= (nmath::scalar_t)EPSILON) return 0.0;

    const nmath::scalar_t vx = nmath::dot(v, t);
    const nmath::scalar_t vy = nmath::dot(v, b);
    const nmath::scalar_t vz2 = cos_theta * cos_theta;
    const nmath::scalar_t ax = clamp_alpha(alpha_x);
    const nmath::scalar_t ay = clamp_alpha(alpha_y);
    const nmath::scalar_t lambda_term = ((vx * vx) * (ax * ax) + (vy * vy) * (ay * ay)) / std::max((nmath::scalar_t)EPSILON, vz2);
    return ((nmath::scalar_t)2.0 / ((nmath::scalar_t)1.0 + nmath_sqrt((nmath::scalar_t)1.0 + lambda_term)));
}

nmath::scalar_t smith_ggx_g(const nmath::Vector3f &normal,
                            const nmath::Vector3f &wo,
                            const nmath::Vector3f &wi,
                            nmath::scalar_t roughness)
{
    return smith_ggx_g1(normal, wo, roughness) * smith_ggx_g1(normal, wi, roughness);
}

nmath::scalar_t smith_ggx_g_anisotropic(const nmath::Vector3f &normal,
                                        const nmath::Vector3f &tangent,
                                        const nmath::Vector3f &bitangent,
                                        const nmath::Vector3f &wo,
                                        const nmath::Vector3f &wi,
                                        nmath::scalar_t alpha_x,
                                        nmath::scalar_t alpha_y)
{
    return smith_ggx_g1_anisotropic(normal, tangent, bitangent, wo, alpha_x, alpha_y)
         * smith_ggx_g1_anisotropic(normal, tangent, bitangent, wi, alpha_x, alpha_y);
}

nmath::Vector3f sample_ggx_half_vector(const nmath::Vector3f &normal,
                                       nmath::scalar_t roughness,
                                       nmath::scalar_t &out_pdf)
{
    const nmath::scalar_t alpha = clamp_roughness(roughness);
    const nmath::scalar_t u1 = nmath::prng_c(0.0, 1.0);
    const nmath::scalar_t u2 = nmath::prng_c(0.0, 1.0);

    const nmath::scalar_t phi = ((nmath::scalar_t)2.0 * nmath::PI) * u1;
    const nmath::scalar_t a2 = alpha * alpha;
    const nmath::scalar_t denom = std::max((nmath::scalar_t)EPSILON, (nmath::scalar_t)1.0 + (a2 - (nmath::scalar_t)1.0) * u2);
    const nmath::scalar_t cos_theta = nmath_sqrt(std::max((nmath::scalar_t)0.0, ((nmath::scalar_t)1.0 - u2) / denom));
    const nmath::scalar_t sin_theta = nmath_sqrt(std::max((nmath::scalar_t)0.0, (nmath::scalar_t)1.0 - cos_theta * cos_theta));

    const nmath::Vector3f n = normal.normalized();
    const nmath::Vector3f t = build_tangent(n);
    const nmath::Vector3f b = nmath::cross(n, t).normalized();

    nmath::Vector3f h = (t * (sin_theta * nmath_cos(phi)))
                      + (n * cos_theta)
                      + (b * (sin_theta * nmath_sin(phi)));
    h.normalize();

    const nmath::scalar_t d = ggx_ndf(n, h, roughness);
    const nmath::scalar_t nh = std::max((nmath::scalar_t)0.0, nmath::dot(n, h));
    out_pdf = d * nh;
    return h;
}

nmath::Vector3f sample_ggx_half_vector_anisotropic(const nmath::Vector3f &normal,
                                                   const nmath::Vector3f &tangent,
                                                   const nmath::Vector3f &bitangent,
                                                   nmath::scalar_t alpha_x,
                                                   nmath::scalar_t alpha_y,
                                                   nmath::scalar_t &out_pdf)
{
    const nmath::scalar_t ax = clamp_alpha(alpha_x);
    const nmath::scalar_t ay = clamp_alpha(alpha_y);
    const nmath::scalar_t u1 = nmath::prng_c(0.0, 1.0);
    const nmath::scalar_t u2 = nmath::prng_c(0.0, 1.0);
    const nmath::scalar_t phi = nmath_atan2(ay * nmath_sin(((nmath::scalar_t)2.0 * nmath::PI) * u1),
                                            ax * nmath_cos(((nmath::scalar_t)2.0 * nmath::PI) * u1));
    const nmath::scalar_t cos_phi = nmath_cos(phi);
    const nmath::scalar_t sin_phi = nmath_sin(phi);
    const nmath::scalar_t denom = (cos_phi * cos_phi) / (ax * ax) + (sin_phi * sin_phi) / (ay * ay);
    const nmath::scalar_t tan2_theta = u2 / std::max((nmath::scalar_t)EPSILON, (((nmath::scalar_t)1.0 - u2) * denom));
    const nmath::scalar_t cos_theta = ((nmath::scalar_t)1.0 / nmath_sqrt((nmath::scalar_t)1.0 + tan2_theta));
    const nmath::scalar_t sin_theta = nmath_sqrt(std::max((nmath::scalar_t)0.0, (nmath::scalar_t)1.0 - cos_theta * cos_theta));

    const nmath::Vector3f n = normal.normalized();
    const nmath::Vector3f t = tangent.normalized();
    const nmath::Vector3f b = bitangent.normalized();
    nmath::Vector3f h = (t * (sin_theta * cos_phi))
                      + (b * (sin_theta * sin_phi))
                      + (n * cos_theta);
    h.normalize();

    const nmath::scalar_t d = ggx_ndf_anisotropic(n, t, b, h, ax, ay);
    const nmath::scalar_t nh = std::max((nmath::scalar_t)0.0, nmath::dot(n, h));
    out_pdf = d * nh;
    return h;
}

nmath::scalar_t fresnel_dielectric(nmath::scalar_t cos_theta_i,
                                   nmath::scalar_t eta_i,
                                   nmath::scalar_t eta_t)
{
    cos_theta_i = clamp_scalar(cos_theta_i, (nmath::scalar_t)-1.0, (nmath::scalar_t)1.0);
    bool entering = cos_theta_i >= (nmath::scalar_t)0.0;
    if (!entering) {
        std::swap(eta_i, eta_t);
        cos_theta_i = nmath_abs(cos_theta_i);
    }

    const nmath::scalar_t sin_theta_i = nmath_sqrt(std::max((nmath::scalar_t)0.0, (nmath::scalar_t)1.0 - cos_theta_i * cos_theta_i));
    const nmath::scalar_t sin_theta_t = eta_i / std::max((nmath::scalar_t)EPSILON, eta_t) * sin_theta_i;
    if (sin_theta_t >= (nmath::scalar_t)1.0) return 1.0;

    const nmath::scalar_t cos_theta_t = nmath_sqrt(std::max((nmath::scalar_t)0.0, (nmath::scalar_t)1.0 - sin_theta_t * sin_theta_t));
    const nmath::scalar_t r_parl = ((eta_t * cos_theta_i) - (eta_i * cos_theta_t))
                                 / std::max((nmath::scalar_t)EPSILON, (eta_t * cos_theta_i) + (eta_i * cos_theta_t));
    const nmath::scalar_t r_perp = ((eta_i * cos_theta_i) - (eta_t * cos_theta_t))
                                 / std::max((nmath::scalar_t)EPSILON, (eta_i * cos_theta_i) + (eta_t * cos_theta_t));
    return (r_parl * r_parl + r_perp * r_perp) * (nmath::scalar_t)0.5;
}

nimg::ColorRGBf fresnel_schlick(const nimg::ColorRGBf &f0, nmath::scalar_t cos_theta)
{
    const nmath::scalar_t ct = clamp_scalar(cos_theta, (nmath::scalar_t)0.0, (nmath::scalar_t)1.0);
    const nmath::scalar_t k = nmath_pow((nmath::scalar_t)1.0 - ct, (nmath::scalar_t)5.0);
    return f0 + (nimg::ColorRGBf(1.0f, 1.0f, 1.0f) - f0) * k;
}

} // namespace sampling
} // namespace math
} // namespace xtcore
