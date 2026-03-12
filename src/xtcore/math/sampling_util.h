#ifndef XTCORE_MATH_SAMPLING_UTIL_H_INCLUDED
#define XTCORE_MATH_SAMPLING_UTIL_H_INCLUDED

#include <algorithm>

#include <nmath/precision.h>
#include <nmath/prng.h>
#include <nmath/vector.h>

namespace xtcore {
namespace math {
namespace sampling {

inline nmath::Vector3f build_tangent(const nmath::Vector3f &n)
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

inline nmath::Vector3f sample_cosine_hemisphere(const nmath::Vector3f &normal, nmath::scalar_t &out_pdf)
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

inline nmath::Vector3f sample_power_cosine_lobe(const nmath::Vector3f &axis,
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

} // namespace sampling
} // namespace math
} // namespace xtcore

#endif /* XTCORE_MATH_SAMPLING_UTIL_H_INCLUDED */
