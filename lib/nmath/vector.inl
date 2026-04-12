/*

    This file is part of the libnmath.

    vector.inl
    Vector inline functions

    Copyright (C) 2008, 2010, 2011
    Papadopoulos Nikolaos

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General
    Public License along with this library; if not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301 USA

*/

#ifndef NMATH_VECTOR_INL_INCLUDED
#define NMATH_VECTOR_INL_INCLUDED

#ifndef NMATH_VECTOR_H_INCLUDED
    #error "vector.h must be included before vector.inl"
#endif /* NMATH_VECTOR_H_INCLUDED */

#include "precision.h"
#include "types.h"
#include "mutil.h"

#include <cmath>
#if defined(NMATH_ENABLE_SIMD) && !defined(MATH_SINGLE_PRECISION)
    #if defined(NMATH_ENABLE_SIMD_AVX) && defined(__AVX__)
        #include <immintrin.h>
        #define NMATH_SIMD_DOUBLE_BATCH3_AVX 1
    #elif defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && (_M_IX86_FP >= 2))
        #include <emmintrin.h>
        #define NMATH_SIMD_DOUBLE_BATCH3_SSE2 1
    #endif
#endif
#ifndef NMATH_ENABLE_VEC3_MANUAL_SIMD
    #define NMATH_ENABLE_VEC3_MANUAL_SIMD 0
#endif

#if defined(NMATH_ENABLE_SIMD) && !defined(MATH_SINGLE_PRECISION) && NMATH_ENABLE_VEC3_MANUAL_SIMD
    #if defined(NMATH_ENABLE_SIMD_AVX) && defined(__AVX__)
        #define NMATH_SIMD_DOUBLE_VEC3_AVX 1
    #elif defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && (_M_IX86_FP >= 2))
        #define NMATH_SIMD_DOUBLE_VEC3_SSE2 1
    #endif
#endif
#ifndef NMATH_SIMD_DOUBLE_VEC3_AVX
    #define NMATH_SIMD_DOUBLE_VEC3_AVX 0
#endif
#ifndef NMATH_SIMD_DOUBLE_VEC3_SSE2
    #define NMATH_SIMD_DOUBLE_VEC3_SSE2 0
#endif
#if NMATH_SIMD_DOUBLE_VEC3_AVX || NMATH_SIMD_DOUBLE_VEC3_SSE2
    #define NMATH_SIMD_DOUBLE_VEC3 1
#else
    #define NMATH_SIMD_DOUBLE_VEC3 0
#endif
#ifndef NMATH_SIMD_DOUBLE_VEC3
    #define NMATH_SIMD_DOUBLE_VEC3 0
#endif
#ifndef NMATH_SIMD_DOUBLE_BATCH3_AVX
    #define NMATH_SIMD_DOUBLE_BATCH3_AVX 0
#endif
#ifndef NMATH_SIMD_DOUBLE_BATCH3_SSE2
    #define NMATH_SIMD_DOUBLE_BATCH3_SSE2 0
#endif
#if NMATH_SIMD_DOUBLE_BATCH3_AVX || NMATH_SIMD_DOUBLE_BATCH3_SSE2
    #define NMATH_SIMD_DOUBLE_BATCH3 1
#else
    #define NMATH_SIMD_DOUBLE_BATCH3 0
#endif

#include "matrix.h"

namespace nmath {

namespace detail {
#if NMATH_SIMD_DOUBLE_VEC3_AVX
inline __m256d simd_v3_pack(const Vector3f &v)
{
    return _mm256_set_pd(0.0, v.z, v.y, v.x);
}

inline Vector3f simd_unpack_v3(__m256d v)
{
    const __m128d lo = _mm256_castpd256_pd128(v);
    const __m128d hi = _mm256_extractf128_pd(v, 1);
    return Vector3f(
        _mm_cvtsd_f64(lo),
        _mm_cvtsd_f64(_mm_unpackhi_pd(lo, lo)),
        _mm_cvtsd_f64(hi)
    );
}

inline scalar_t simd_dot3(const Vector3f &v1, const Vector3f &v2)
{
    const __m256d mul = _mm256_mul_pd(simd_v3_pack(v1), simd_v3_pack(v2));
    const __m128d lo = _mm256_castpd256_pd128(mul);
    const __m128d hi = _mm256_extractf128_pd(mul, 1);
    return _mm_cvtsd_f64(_mm_add_sd(lo, _mm_unpackhi_pd(lo, lo))) + _mm_cvtsd_f64(hi);
}

inline Vector3f simd_op3_add(const Vector3f &v1, const Vector3f &v2)
{
    return simd_unpack_v3(_mm256_add_pd(simd_v3_pack(v1), simd_v3_pack(v2)));
}

inline Vector3f simd_op3_sub(const Vector3f &v1, const Vector3f &v2)
{
    return simd_unpack_v3(_mm256_sub_pd(simd_v3_pack(v1), simd_v3_pack(v2)));
}

inline Vector3f simd_op3_mul(const Vector3f &v1, const Vector3f &v2)
{
    return simd_unpack_v3(_mm256_mul_pd(simd_v3_pack(v1), simd_v3_pack(v2)));
}

inline Vector3f simd_op3_div(const Vector3f &v1, const Vector3f &v2)
{
    return simd_unpack_v3(_mm256_div_pd(simd_v3_pack(v1), simd_v3_pack(v2)));
}

inline Vector3f simd_op3_scale(const Vector3f &v, scalar_t scalar)
{
    return simd_unpack_v3(_mm256_mul_pd(simd_v3_pack(v), _mm256_set1_pd(scalar)));
}
#elif NMATH_SIMD_DOUBLE_VEC3_SSE2
inline __m128d simd_v3_xy(const Vector3f &v)
{
    return _mm_set_pd(v.y, v.x);
}

inline __m128d simd_v3_z(const Vector3f &v)
{
    return _mm_set_sd(v.z);
}

inline scalar_t simd_hsum2(__m128d v)
{
    return _mm_cvtsd_f64(_mm_add_sd(v, _mm_unpackhi_pd(v, v)));
}

inline scalar_t simd_dot3(const Vector3f &v1, const Vector3f &v2)
{
    const __m128d xy1 = simd_v3_xy(v1);
    const __m128d xy2 = simd_v3_xy(v2);
    const __m128d z1 = simd_v3_z(v1);
    const __m128d z2 = simd_v3_z(v2);
    return simd_hsum2(_mm_mul_pd(xy1, xy2)) + _mm_cvtsd_f64(_mm_mul_sd(z1, z2));
}

inline Vector3f simd_op3_add(const Vector3f &v1, const Vector3f &v2)
{
    const __m128d xy = _mm_add_pd(simd_v3_xy(v1), simd_v3_xy(v2));
    const __m128d z = _mm_add_sd(simd_v3_z(v1), simd_v3_z(v2));
    return Vector3f(_mm_cvtsd_f64(xy), _mm_cvtsd_f64(_mm_unpackhi_pd(xy, xy)), _mm_cvtsd_f64(z));
}

inline Vector3f simd_op3_sub(const Vector3f &v1, const Vector3f &v2)
{
    const __m128d xy = _mm_sub_pd(simd_v3_xy(v1), simd_v3_xy(v2));
    const __m128d z = _mm_sub_sd(simd_v3_z(v1), simd_v3_z(v2));
    return Vector3f(_mm_cvtsd_f64(xy), _mm_cvtsd_f64(_mm_unpackhi_pd(xy, xy)), _mm_cvtsd_f64(z));
}

inline Vector3f simd_op3_mul(const Vector3f &v1, const Vector3f &v2)
{
    const __m128d xy = _mm_mul_pd(simd_v3_xy(v1), simd_v3_xy(v2));
    const __m128d z = _mm_mul_sd(simd_v3_z(v1), simd_v3_z(v2));
    return Vector3f(_mm_cvtsd_f64(xy), _mm_cvtsd_f64(_mm_unpackhi_pd(xy, xy)), _mm_cvtsd_f64(z));
}

inline Vector3f simd_op3_div(const Vector3f &v1, const Vector3f &v2)
{
    const __m128d xy = _mm_div_pd(simd_v3_xy(v1), simd_v3_xy(v2));
    const __m128d z = _mm_div_sd(simd_v3_z(v1), simd_v3_z(v2));
    return Vector3f(_mm_cvtsd_f64(xy), _mm_cvtsd_f64(_mm_unpackhi_pd(xy, xy)), _mm_cvtsd_f64(z));
}

inline Vector3f simd_op3_scale(const Vector3f &v, scalar_t scalar)
{
    const __m128d scale2 = _mm_set1_pd(scalar);
    const __m128d xy = _mm_mul_pd(simd_v3_xy(v), scale2);
    const __m128d z = _mm_mul_sd(simd_v3_z(v), _mm_set_sd(scalar));
    return Vector3f(_mm_cvtsd_f64(xy), _mm_cvtsd_f64(_mm_unpackhi_pd(xy, xy)), _mm_cvtsd_f64(z));
}
#endif
} /* namespace detail */

/* Vector2f functions */
inline scalar_t &Vector2f::operator [](unsigned int index)
{
	return index ? y : x;
}

inline const scalar_t &Vector2f::operator [](unsigned int index) const
{
	return index ? y : x;
}

inline const Vector2f& Vector2f::operator =(const Vector2f& v)
{
    x = v.x;
    y = v.y;
    return *this;
}

inline const Vector2f operator -(const Vector2f& v)
{
	return Vector2f(-v.x, -v.y);
}

inline const Vector2f operator +(const Vector2f& v1, const Vector2f& v2)
{
	return Vector2f(v1.x + v2.x, v1.y + v2.y);
}

inline const Vector2f operator -(const Vector2f& v1, const Vector2f& v2)
{
	return Vector2f(v1.x - v2.x, v1.y - v2.y);
}

inline const Vector2f operator *(const Vector2f& v1, const Vector2f& v2)
{
	return Vector2f(v1.x * v2.x, v1.y * v2.y);
}

inline const Vector2f operator /(const Vector2f& v1, const Vector2f& v2)
{
	return Vector2f(v1.x / v2.x, v1.y / v2.y);
}

inline const Vector2f operator +(const Vector2f& v, scalar_t r)
{
	return Vector2f(v.x + r, v.y + r);
}

inline const Vector2f operator +(scalar_t r, const Vector2f& v)
{
	return Vector2f(v.x + r, v.y + r);
}

inline const Vector2f operator -(const Vector2f& v, scalar_t r)
{
	return Vector2f(v.x - r, v.y - r);
}

inline const Vector2f operator *(const Vector2f& v, scalar_t r)
{
	return Vector2f(v.x * r, v.y * r);
}

inline const Vector2f operator *(scalar_t r, const Vector2f& v)
{
	return Vector2f(v.x * r, v.y * r);
}

inline const Vector2f operator /(const Vector2f& v, scalar_t r)
{
	return Vector2f(v.x / r, v.y / r);
}

inline Vector2f& operator +=(Vector2f& v1, const Vector2f& v2)
{
	v1.x += v2.x;
	v1.y += v2.y;
	return v1;
}

inline Vector2f& operator -=(Vector2f& v1, const Vector2f& v2)
{
	v1.x -= v2.x;
	v1.y -= v2.y;
	return v1;
}

inline Vector2f& operator *=(Vector2f& v1, const Vector2f& v2)
{
	v1.x *= v2.x;
	v1.y *= v2.y;
	return v1;
}

inline Vector2f& operator /=(Vector2f& v1, const Vector2f& v2)
{
	v1.x /= v2.x;
	v1.y /= v2.y;
	return v1;
}

inline Vector2f& operator +=(Vector2f& v, scalar_t r)
{
	v.x += r;
	v.y += r;
	return v;
}

inline Vector2f& operator -=(Vector2f& v, scalar_t r)
{
	v.x -= r;
	v.y -= r;
	return v;
}

inline Vector2f& operator *=(Vector2f& v, scalar_t r)
{
	v.x *= r;
	v.y *= r;
	return v;
}

inline Vector2f& operator /=(Vector2f& v, scalar_t r)
{
	v.x /= r;
	v.y /= r;
	return v;
}

inline bool operator ==(const Vector2f& v1, const Vector2f& v2)
{
	return (fabs(v1.x - v2.x) < SCALAR_XXSMALL) && (fabs(v1.y - v2.y) < SCALAR_XXSMALL);
}

inline bool operator !=(const Vector2f& v1, const Vector2f& v2)
{
	return (fabs(v1.x - v2.x) >= SCALAR_XXSMALL) || (fabs(v1.y - v2.y) >= SCALAR_XXSMALL);
}

inline scalar_t Vector2f::length() const
{
	return (scalar_t)sqrt(x*x + y*y);
}

inline scalar_t Vector2f::length_squared() const
{
	return x*x + y*y;
}

inline void Vector2f::normalize()
{
    const scalar_t len_sq = length_squared();
    if (len_sq == 0) return;
    if (nmath_abs(len_sq - (scalar_t)1.0) <= SCALAR_XSMALL) return;
    const scalar_t inv_len = (scalar_t)1.0 / nmath_sqrt(len_sq);
	x *= inv_len;
	y *= inv_len;
}

inline Vector2f Vector2f::normalized() const
{
    const scalar_t len_sq = length_squared();
    if (len_sq == 0) return *this;
    if (nmath_abs(len_sq - (scalar_t)1.0) <= SCALAR_XSMALL) return *this;
    const scalar_t inv_len = (scalar_t)1.0 / nmath_sqrt(len_sq);
    return Vector2f(x * inv_len, y * inv_len);
}

inline void Vector2f::reflect(const Vector2f &normal)
{
	*this = reflected(normal);
}

inline Vector2f Vector2f::reflected(const Vector2f &normal) const
{
	Vector2f i = normalized();
	Vector2f n = normal.normalized();
	return (2 * dot(i, n) * n) - i;
}

inline void Vector2f::refract(const Vector2f &normal, scalar_t ior_src, scalar_t ior_dst)
{
	*this = refracted(normal, ior_src, ior_dst);
}

inline Vector2f Vector2f::refracted(const Vector2f &normal, scalar_t ior_src, scalar_t ior_dst) const
{
	Vector2f n = normal.normalized();
	Vector2f i = normalized();
	scalar_t ior = ior_src / ior_dst;

	scalar_t cos_inc = - dot(n, i);
	scalar_t radical = 1.f - ((ior * ior) * (1.f - (cos_inc * cos_inc)));

	if(radical < 0.f)
	{
		/* total internal reflection */
		return reflected(n);
	}

	scalar_t beta = ior * cos_inc - sqrt(radical);

	return (ior * i) + (beta * n);
}

inline Vector2f Vector2f::transform(Matrix3x3f &m)
{
	return *this = transformed(m);
}

inline Vector2f Vector2f::transformed(Matrix3x3f &m)
{
	scalar_t nx = m.data[0][0] * x + m.data[0][1]* y + m.data[0][2];
	scalar_t ny = m.data[1][0] * x + m.data[1][1]* y + m.data[1][2];
	return Vector2f(nx, ny);
}

inline scalar_t dot(const Vector2f& v1, const Vector2f& v2)
{
	return v1.x * v2.x + v1.y * v2.y;
}

/* Vector3f functions */
inline scalar_t& Vector3f::operator [](unsigned int index)
{
	return index ? (index == 1 ? y : z) : x;
}

inline const scalar_t& Vector3f::operator [](unsigned int index) const
{
	return index ? (index == 1 ? y : z) : x;
}

inline const Vector3f& Vector3f::operator =(const Vector3f& v)
{
    x = v.x;
    y = v.y;
    z = v.z;
    return *this;
}

inline const Vector3f operator -(const Vector3f& v)
{
	return Vector3f(-v.x, -v.y, -v.z);
}

inline const Vector3f operator +(const Vector3f& v1, const Vector3f& v2)
{
#if NMATH_SIMD_DOUBLE_VEC3
    return detail::simd_op3_add(v1, v2);
#else
	return Vector3f(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
#endif
}

inline const Vector3f operator -(const Vector3f& v1, const Vector3f& v2)
{
#if NMATH_SIMD_DOUBLE_VEC3
    return detail::simd_op3_sub(v1, v2);
#else
	return Vector3f(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
#endif
}

inline const Vector3f operator *(const Vector3f& v1, const Vector3f& v2)
{
#if NMATH_SIMD_DOUBLE_VEC3
    return detail::simd_op3_mul(v1, v2);
#else
	return Vector3f(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z);
#endif
}

inline const Vector3f operator /(const Vector3f& v1, const Vector3f& v2)
{
#if NMATH_SIMD_DOUBLE_VEC3
    return detail::simd_op3_div(v1, v2);
#else
	return Vector3f(v1.x / v2.x, v1.y / v2.y, v1.z / v2.z);
#endif
}

inline const Vector3f operator +(const Vector3f& v, scalar_t r)
{
	return Vector3f(v.x + r, v.y + r, v.z + r);
}

inline const Vector3f operator +(scalar_t r, const Vector3f& v)
{
	return Vector3f(v.x + r, v.y + r, v.z + r);
}

inline const Vector3f operator -(const Vector3f& v, scalar_t r)
{
	return Vector3f(v.x - r, v.y - r, v.z - r);
}

inline const Vector3f operator *(const Vector3f& v, scalar_t r)
{
	return Vector3f(v.x * r, v.y * r, v.z * r);
}

inline const Vector3f operator *(scalar_t r, const Vector3f& v)
{
	return Vector3f(v.x * r, v.y * r, v.z * r);
}

inline const Vector3f operator /(const Vector3f& v, scalar_t r)
{
	return Vector3f(v.x / r, v.y / r, v.z / r);
}

inline Vector3f& operator +=(Vector3f& v1, const Vector3f& v2)
{
#if NMATH_SIMD_DOUBLE_VEC3
    v1 = detail::simd_op3_add(v1, v2);
    return v1;
#else
	v1.x += v2.x;
	v1.y += v2.y;
	v1.z += v2.z;
	return v1;
#endif
}

inline Vector3f& operator -=(Vector3f& v1, const Vector3f& v2)
{
#if NMATH_SIMD_DOUBLE_VEC3
    v1 = detail::simd_op3_sub(v1, v2);
    return v1;
#else
	v1.x -= v2.x;
	v1.y -= v2.y;
	v1.z -= v2.z;
	return v1;
#endif
}

inline Vector3f& operator *=(Vector3f& v1, const Vector3f& v2)
{
#if NMATH_SIMD_DOUBLE_VEC3
    v1 = detail::simd_op3_mul(v1, v2);
    return v1;
#else
	v1.x *= v2.x;
	v1.y *= v2.y;
	v1.z *= v2.z;
	return v1;
#endif
}

inline Vector3f& operator /=(Vector3f& v1, const Vector3f& v2)
{
#if NMATH_SIMD_DOUBLE_VEC3
    v1 = detail::simd_op3_div(v1, v2);
    return v1;
#else
	v1.x /= v2.x;
	v1.y /= v2.y;
	v1.z /= v2.z;
	return v1;
#endif
}

inline Vector3f& operator +=(Vector3f& v, scalar_t r)
{
	v.x += r;
	v.y += r;
	v.z += r;
	return v;
}

inline Vector3f& operator -=(Vector3f& v, scalar_t r)
{
	v.x -= r;
	v.y -= r;
	v.z -= r;
	return v;
}

inline Vector3f& operator *=(Vector3f& v, scalar_t r)
{
	v.x *= r;
	v.y *= r;
	v.z *= r;
	return v;
}

inline Vector3f& operator /=(Vector3f& v, scalar_t r)
{
	v.x /= r;
	v.y /= r;
	v.z /= r;
	return v;
}

inline bool operator ==(const Vector3f& v1, const Vector3f& v2)
{
	return (fabs(v1.x - v2.x) < SCALAR_XXSMALL) && (fabs(v1.y - v2.y) < SCALAR_XXSMALL) && (fabs(v1.z - v2.z) < SCALAR_XXSMALL);
}

inline bool operator !=(const Vector3f& v1, const Vector3f& v2)
{
	return (fabs(v1.x - v2.x) >= SCALAR_XXSMALL) || (fabs(v1.y - v2.y) >= SCALAR_XXSMALL) || (fabs(v1.z - v2.z) >= SCALAR_XXSMALL);
}

inline bool operator < (const Vector3f &v1, const Vector3f &v2)
{
	return v1.x < v2.x && v1.y < v2.y && v1.z < v2.z;
}

inline bool operator > (const Vector3f &v1, const Vector3f &v2)
{
	return v1.x > v2.x && v1.y > v2.y && v1.z > v2.z;
}

inline scalar_t Vector3f::length() const
{
#if NMATH_SIMD_DOUBLE_VEC3
    return nmath_sqrt(detail::simd_dot3(*this, *this));
#else
	return sqrt(x*x + y*y + z*z);
#endif
}

inline scalar_t Vector3f::length_squared() const
{
#if NMATH_SIMD_DOUBLE_VEC3
    return detail::simd_dot3(*this, *this);
#else
	return x*x + y*y + z*z;
#endif
}

inline void Vector3f::normalize()
{
    const scalar_t len_sq = length_squared();
    if (len_sq == 0) return;
    if (nmath_abs(len_sq - (scalar_t)1.0) <= SCALAR_XSMALL) return;
    const scalar_t inv_len = (scalar_t)1.0 / nmath_sqrt(len_sq);
#if NMATH_SIMD_DOUBLE_VEC3
    *this = detail::simd_op3_scale(*this, inv_len);
#else
	x *= inv_len;
	y *= inv_len;
	z *= inv_len;
#endif
}

inline Vector3f Vector3f::normalized() const
{
    const scalar_t len_sq = length_squared();
    if (len_sq == 0) return *this;
    if (nmath_abs(len_sq - (scalar_t)1.0) <= SCALAR_XSMALL) return *this;
    const scalar_t inv_len = (scalar_t)1.0 / nmath_sqrt(len_sq);
#if NMATH_SIMD_DOUBLE_VEC3
    return detail::simd_op3_scale(*this, inv_len);
#else
    return Vector3f(x * inv_len, y * inv_len, z * inv_len);
#endif
}

inline void Vector3f::reflect(const Vector3f &normal)
{
	*this = reflected(normal);
}

inline Vector3f Vector3f::reflected(const Vector3f &normal) const
{
	Vector3f i = normalized();
	Vector3f n = normal.normalized();
	return (2 * dot(i, n) * n) - i;
}

inline void Vector3f::refract(const Vector3f &normal, scalar_t ior_src, scalar_t ior_dst)
{
	*this = refracted(normal, ior_src, ior_dst);
}

inline Vector3f Vector3f::refracted(const Vector3f &normal, scalar_t ior_src, scalar_t ior_dst) const
{
	Vector3f n = normal.normalized();
	Vector3f i = normalized();

	scalar_t cos_inc = dot(i, -n);

	scalar_t ior = ior_src / ior_dst;

	scalar_t radical = 1.f + ((ior * ior) * ((cos_inc * cos_inc) - 1.0));

	if(radical < 0.f)
	{
		/* total internal reflection */
		return -reflected(n);
	}

	scalar_t beta = ior * cos_inc - sqrt(radical);

	return (ior * i) + (beta * n);
}

inline scalar_t dot(const Vector3f& v1, const Vector3f& v2)
{
#if NMATH_SIMD_DOUBLE_VEC3
    return detail::simd_dot3(v1, v2);
#else
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
#endif
}

inline Vector3f cross(const Vector3f& v1, const Vector3f& v2)
{
	return Vector3f(v1.y * v2.z - v1.z * v2.y,  v1.z * v2.x - v1.x * v2.z,  v1.x * v2.y - v1.y * v2.x);
}

namespace batch {

inline void vec3_normalize_soa(
    const scalar_t *in_x,
    const scalar_t *in_y,
    const scalar_t *in_z,
    scalar_t *out_x,
    scalar_t *out_y,
    scalar_t *out_z,
    std::size_t count)
{
    std::size_t i = 0;
#if NMATH_SIMD_DOUBLE_BATCH3_AVX
    const __m256d zero = _mm256_setzero_pd();
    const __m256d one = _mm256_set1_pd(1.0);
    for (; (i + 4) <= count; i += 4) {
        const __m256d x = _mm256_loadu_pd(in_x + i);
        const __m256d y = _mm256_loadu_pd(in_y + i);
        const __m256d z = _mm256_loadu_pd(in_z + i);
        const __m256d len2 = _mm256_add_pd(_mm256_add_pd(_mm256_mul_pd(x, x), _mm256_mul_pd(y, y)), _mm256_mul_pd(z, z));
        const __m256d mask = _mm256_cmp_pd(len2, zero, _CMP_GT_OQ);
        const __m256d inv_len = _mm256_blendv_pd(zero, _mm256_div_pd(one, _mm256_sqrt_pd(len2)), mask);
        _mm256_storeu_pd(out_x + i, _mm256_mul_pd(x, inv_len));
        _mm256_storeu_pd(out_y + i, _mm256_mul_pd(y, inv_len));
        _mm256_storeu_pd(out_z + i, _mm256_mul_pd(z, inv_len));
    }
#elif NMATH_SIMD_DOUBLE_BATCH3_SSE2
    const __m128d zero = _mm_setzero_pd();
    const __m128d one = _mm_set1_pd(1.0);
    for (; (i + 2) <= count; i += 2) {
        const __m128d x = _mm_loadu_pd(in_x + i);
        const __m128d y = _mm_loadu_pd(in_y + i);
        const __m128d z = _mm_loadu_pd(in_z + i);
        const __m128d len2 = _mm_add_pd(_mm_add_pd(_mm_mul_pd(x, x), _mm_mul_pd(y, y)), _mm_mul_pd(z, z));
        const __m128d mask = _mm_cmpgt_pd(len2, zero);
        const __m128d inv_len = _mm_and_pd(_mm_div_pd(one, _mm_sqrt_pd(len2)), mask);
        _mm_storeu_pd(out_x + i, _mm_mul_pd(x, inv_len));
        _mm_storeu_pd(out_y + i, _mm_mul_pd(y, inv_len));
        _mm_storeu_pd(out_z + i, _mm_mul_pd(z, inv_len));
    }
#endif
    for (; i < count; ++i) {
        const scalar_t x = in_x[i];
        const scalar_t y = in_y[i];
        const scalar_t z = in_z[i];
        const scalar_t len2 = x * x + y * y + z * z;
        if (len2 > (scalar_t)0.0) {
            const scalar_t inv_len = (scalar_t)1.0 / nmath_sqrt(len2);
            out_x[i] = x * inv_len;
            out_y[i] = y * inv_len;
            out_z[i] = z * inv_len;
        } else {
            out_x[i] = (scalar_t)0.0;
            out_y[i] = (scalar_t)0.0;
            out_z[i] = (scalar_t)0.0;
        }
    }
}

inline void vec3_dot_cross_soa(
    const scalar_t *ax,
    const scalar_t *ay,
    const scalar_t *az,
    const scalar_t *bx,
    const scalar_t *by,
    const scalar_t *bz,
    scalar_t *out_dot,
    scalar_t *out_cx,
    scalar_t *out_cy,
    scalar_t *out_cz,
    std::size_t count)
{
    std::size_t i = 0;
#if NMATH_SIMD_DOUBLE_BATCH3_AVX
    for (; (i + 4) <= count; i += 4) {
        const __m256d avx = _mm256_loadu_pd(ax + i);
        const __m256d avy = _mm256_loadu_pd(ay + i);
        const __m256d avz = _mm256_loadu_pd(az + i);
        const __m256d bvx = _mm256_loadu_pd(bx + i);
        const __m256d bvy = _mm256_loadu_pd(by + i);
        const __m256d bvz = _mm256_loadu_pd(bz + i);
        _mm256_storeu_pd(out_dot + i, _mm256_add_pd(_mm256_add_pd(_mm256_mul_pd(avx, bvx), _mm256_mul_pd(avy, bvy)), _mm256_mul_pd(avz, bvz)));
        _mm256_storeu_pd(out_cx + i, _mm256_sub_pd(_mm256_mul_pd(avy, bvz), _mm256_mul_pd(avz, bvy)));
        _mm256_storeu_pd(out_cy + i, _mm256_sub_pd(_mm256_mul_pd(avz, bvx), _mm256_mul_pd(avx, bvz)));
        _mm256_storeu_pd(out_cz + i, _mm256_sub_pd(_mm256_mul_pd(avx, bvy), _mm256_mul_pd(avy, bvx)));
    }
#elif NMATH_SIMD_DOUBLE_BATCH3_SSE2
    for (; (i + 2) <= count; i += 2) {
        const __m128d avx = _mm_loadu_pd(ax + i);
        const __m128d avy = _mm_loadu_pd(ay + i);
        const __m128d avz = _mm_loadu_pd(az + i);
        const __m128d bvx = _mm_loadu_pd(bx + i);
        const __m128d bvy = _mm_loadu_pd(by + i);
        const __m128d bvz = _mm_loadu_pd(bz + i);
        _mm_storeu_pd(out_dot + i, _mm_add_pd(_mm_add_pd(_mm_mul_pd(avx, bvx), _mm_mul_pd(avy, bvy)), _mm_mul_pd(avz, bvz)));
        _mm_storeu_pd(out_cx + i, _mm_sub_pd(_mm_mul_pd(avy, bvz), _mm_mul_pd(avz, bvy)));
        _mm_storeu_pd(out_cy + i, _mm_sub_pd(_mm_mul_pd(avz, bvx), _mm_mul_pd(avx, bvz)));
        _mm_storeu_pd(out_cz + i, _mm_sub_pd(_mm_mul_pd(avx, bvy), _mm_mul_pd(avy, bvx)));
    }
#endif
    for (; i < count; ++i) {
        out_dot[i] = ax[i] * bx[i] + ay[i] * by[i] + az[i] * bz[i];
        out_cx[i] = ay[i] * bz[i] - az[i] * by[i];
        out_cy[i] = az[i] * bx[i] - ax[i] * bz[i];
        out_cz[i] = ax[i] * by[i] - ay[i] * bx[i];
    }
}

} /* namespace batch */

inline Vector3f Vector3f::transform(Matrix3x3f &m)
{
	return *this = transformed(m);
}

inline Vector3f Vector3f::transformed(Matrix3x3f &m)
{
	scalar_t nx = m.data[0][0] * x + m.data[0][1] * y + m.data[0][2] * z;
	scalar_t ny = m.data[1][0] * x + m.data[1][1] * y + m.data[1][2] * z;
	scalar_t nz = m.data[2][0] * x + m.data[2][1] * y + m.data[2][2] * z;
	return Vector3f(nx, ny, nz);
}

inline Vector3f Vector3f::transform(Matrix4x4f &m)
{
	return *this = transformed(m);
}

inline Vector3f Vector3f::transformed(Matrix4x4f &m)
{
	scalar_t nx = m.data[0][0] * x + m.data[0][1] * y + m.data[0][2] * z + m.data[0][3];
	scalar_t ny = m.data[1][0] * x + m.data[1][1] * y + m.data[1][2] * z + m.data[1][3];
	scalar_t nz = m.data[2][0] * x + m.data[2][1] * y + m.data[2][2] * z + m.data[2][3];
	return Vector3f(nx, ny, nz);
}

/* Vector4f functions */
inline scalar_t& Vector4f::operator [](unsigned int index)
{
	return index ? (index == 1 ? y : (index == 2 ? z : w)) : x;
}

inline const scalar_t& Vector4f::operator [](unsigned int index) const
{
	return index ? (index == 1 ? y : (index == 2 ? z : w)) : x;
}

inline const Vector4f& Vector4f::operator =(const Vector4f& v)
{
    x = v.x;
    y = v.y;
    z = v.z;
    w = v.w;
    return *this;
}

inline const Vector4f operator -(const Vector4f& v)
{
	return Vector4f(-v.x, -v.y, -v.z, -v.w);
}

inline const Vector4f operator +(const Vector4f& v1, const Vector4f& v2)
{
	return Vector4f(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w);
}

inline const Vector4f operator -(const Vector4f& v1, const Vector4f& v2)
{
	return Vector4f(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w);
}

inline const Vector4f operator *(const Vector4f& v1, const Vector4f& v2)
{
	return Vector4f(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w);
}

inline const Vector4f operator /(const Vector4f& v1, const Vector4f& v2)
{
	return Vector4f(v1.x / v2.x, v1.y / v2.y, v1.z / v2.z, v1.w / v2.w);
}

inline const Vector4f operator +(const Vector4f& v, scalar_t r)
{
	return Vector4f(v.x + r, v.y + r, v.z + r, v.w + r);
}

inline const Vector4f operator +(scalar_t r, const Vector4f& v)
{
	return Vector4f(v.x + r, v.y + r, v.z + r, v.w + r);
}

inline const Vector4f operator -(const Vector4f& v, scalar_t r)
{
	return Vector4f(v.x - r, v.y - r, v.z - r, v.w - r);
}

inline const Vector4f operator *(const Vector4f& v, scalar_t r)
{
	return Vector4f(v.x * r, v.y * r, v.z * r, v.w * r);
}

inline const Vector4f operator *(scalar_t r, const Vector4f& v)
{
	return Vector4f(v.x * r, v.y * r, v.z * r, v.w * r);
}

inline const Vector4f operator /(const Vector4f& v, scalar_t r)
{
	return Vector4f(v.x / r, v.y / r, v.z / r, v.w / r);
}

inline Vector4f& operator +=(Vector4f& v1, const Vector4f& v2)
{
	v1.x += v2.x;
	v1.y += v2.y;
	v1.z += v2.z;
	v1.w += v2.w;
	return v1;
}

inline Vector4f& operator -=(Vector4f& v1, const Vector4f& v2)
{
	v1.x -= v2.x;
	v1.y -= v2.y;
	v1.z -= v2.z;
	v1.w -= v2.w;
	return v1;
}

inline Vector4f& operator *=(Vector4f& v1, const Vector4f& v2)
{
	v1.x *= v2.x;
	v1.y *= v2.y;
	v1.z *= v2.z;
	v1.w *= v2.w;
	return v1;
}

inline Vector4f& operator /=(Vector4f& v1, const Vector4f& v2)
{
	v1.x /= v2.x;
	v1.y /= v2.y;
	v1.z /= v2.z;
	v1.w /= v2.w;
	return v1;
}

inline Vector4f& operator +=(Vector4f& v, scalar_t r)
{
	v.x += r;
	v.y += r;
	v.z += r;
	v.w += r;
	return v;
}

inline Vector4f& operator -=(Vector4f& v, scalar_t r)
{
	v.x -= r;
	v.y -= r;
	v.z -= r;
	v.w -= r;
	return v;
}

inline Vector4f& operator *=(Vector4f& v, scalar_t r)
{
	v.x *= r;
	v.y *= r;
	v.z *= r;
	v.w *= r;
	return v;
}

inline Vector4f& operator /=(Vector4f& v, scalar_t r)
{
	v.x /= r;
	v.y /= r;
	v.z /= r;
	v.w /= r;
	return v;
}

inline bool operator ==(const Vector4f& v1, const Vector4f& v2)
{
	return (fabs(v1.x - v2.x) < SCALAR_XXSMALL) && (fabs(v1.y - v2.y) < SCALAR_XXSMALL) && (fabs(v1.z - v2.z) < SCALAR_XXSMALL) && (fabs(v1.w - v2.w) < SCALAR_XXSMALL);;
}

inline bool operator !=(const Vector4f& v1, const Vector4f& v2)
{
	return (fabs(v1.x - v2.x) >= SCALAR_XXSMALL) || (fabs(v1.y - v2.y) >= SCALAR_XXSMALL) || (fabs(v1.z - v2.z) >= SCALAR_XXSMALL) || (fabs(v1.w - v2.w) >= SCALAR_XXSMALL);
}

inline scalar_t Vector4f::length() const
{
	return sqrt(x*x + y*y + z*z + w*w);
}

inline scalar_t Vector4f::length_squared() const
{
	return x*x + y*y + z*z + w*w;
}

inline void Vector4f::normalize()
{
    const scalar_t len_sq = length_squared();
    if (len_sq == 0) return;
    if (nmath_abs(len_sq - (scalar_t)1.0) <= SCALAR_XSMALL) return;
    const scalar_t inv_len = (scalar_t)1.0 / nmath_sqrt(len_sq);
	x *= inv_len;
	y *= inv_len;
	z *= inv_len;
	w *= inv_len;
}

inline Vector4f Vector4f::normalized() const
{
    const scalar_t len_sq = length_squared();
    if (len_sq == 0) return *this;
    if (nmath_abs(len_sq - (scalar_t)1.0) <= SCALAR_XSMALL) return *this;
    const scalar_t inv_len = (scalar_t)1.0 / nmath_sqrt(len_sq);
    return Vector4f(x * inv_len, y * inv_len, z * inv_len, w * inv_len);
}

inline void Vector4f::reflect(const Vector4f &normal)
{
	*this = reflected(normal);
}

inline Vector4f Vector4f::reflected(const Vector4f &normal) const
{
	Vector4f i = normalized();
	Vector4f n = normal.normalized();
	return (2 * dot(i, n) * n) - i;
}

inline void Vector4f::refract(const Vector4f &normal, scalar_t ior_src, scalar_t ior_dst)
{
	*this = refracted(normal, ior_src, ior_dst);
}

inline Vector4f Vector4f::refracted(const Vector4f &normal, scalar_t ior_src, scalar_t ior_dst) const
{
	Vector4f n = normal.normalized();
	Vector4f i = normalized();
	scalar_t ior = ior_src / ior_dst;

	scalar_t cos_inc = - dot(n, i);
	scalar_t radical = 1.f - ((ior * ior) * (1.f - (cos_inc * cos_inc)));

	if(radical < 0.f)
	{
		/* total internal reflection */
		return reflected(n);
	}

	scalar_t beta = ior * cos_inc - sqrt(radical);

	return (ior * i) + (beta * n);
}

inline scalar_t dot(const Vector4f& v1, const Vector4f& v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
}

} /* namespace nmath */

#endif /* NMATH_VECTOR_INL_INCLUDED */
