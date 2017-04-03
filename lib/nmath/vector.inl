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

#ifdef __cplusplus
    #include <cmath>
#else
    #include <math.h>
#endif  /* __cplusplus */

#include "matrix.h"

namespace NMath {

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

/* C 2D vector functions */
static inline vec2_t vec2_pack(scalar_t x, scalar_t y)
{
	vec2_t v;
	v.x = x;
	v.y = y;
	return v;
}

static inline vec2_t vec2_add(vec2_t v1, vec2_t v2)
{
	v1.x += v2.x;
	v1.y += v2.y;
	return v1;
}

static inline vec2_t vec2_sub(vec2_t v1, vec2_t v2)
{
	v1.x -= v2.x;
	v1.y -= v2.y;
	return v1;
}

static inline vec2_t vec2_neg(vec2_t v)
{
	v.x = -v.x;
	v.y = -v.y;
	return v;
}

static inline vec2_t vec2_mul(vec2_t v1, vec2_t v2)
{
	v1.x *= v2.x;
	v1.y *= v2.y;
	return v1;
}

static inline vec2_t vec2_scale(vec2_t v, scalar_t s)
{
	v.x *= s;
	v.y *= s;
	return v;
}

static inline scalar_t vec2_length(vec2_t v)
{
	return (scalar_t)sqrt(v.x * v.x + v.y * v.y);
}

static inline scalar_t vec2_length_sq(vec2_t v)
{
	return v.x * v.x + v.y * v.y;
}

static inline vec2_t vec2_normalize(vec2_t v)
{
	scalar_t len = (scalar_t)sqrt(v.x * v.x + v.y * v.y);
	v.x /= len;
	v.y /= len;
	return v;
}

static inline scalar_t vec2_dot(vec2_t v1, vec2_t v2)
{
	return v1.x * v2.x + v1.y * v2.y;
}

static inline vec2_t vec2_reflect(vec2_t v, vec2_t n)
{
	vec2_t normal = vec2_normalize(n);
	vec2_t incident = vec2_normalize(v);
	scalar_t val = 2 * vec2_dot(incident, normal);
	return vec2_sub(vec2_scale(normal, val), incident);
}

static inline vec2_t vec2_refract(vec2_t v, vec2_t n, scalar_t ior_src, scalar_t ior_dst)
{
	vec2_t normal = vec2_normalize(n);
	vec2_t incident = vec2_normalize(v);
	scalar_t ior = ior_src / ior_dst;
	scalar_t cos_inc = - vec2_dot(normal, incident);
	scalar_t radical = 1.f - ((ior * ior) * (1.f - (cos_inc * cos_inc)));

	if(radical < 0.f)
	{
		/* total internal reflection */
		return vec2_reflect(v, n);
	}

	scalar_t beta = ior * cos_inc - sqrt(radical);

	return vec2_add( vec2_scale(incident, ior), vec2_scale(normal, beta));
}

static inline void vec2_print(FILE *fp, vec2_t v)
{
	fprintf(fp, "[ %.4f, %.4f ]", v.x, v.y);
}

/* C 3D vector functions */
static inline vec3_t vec3_pack(scalar_t x, scalar_t y, scalar_t z)
{
	vec3_t v;
	v.x = x;
	v.y = y;
	v.z = z;
	return v;
}

static inline vec3_t vec3_add(vec3_t v1, vec3_t v2)
{
	v1.x += v2.x;
	v1.y += v2.y;
	v1.z += v2.z;
	return v1;
}

static inline vec3_t vec3_sub(vec3_t v1, vec3_t v2)
{
	v1.x -= v2.x;
	v1.y -= v2.y;
	v1.z -= v2.z;
	return v1;
}

static inline vec3_t vec3_neg(vec3_t v)
{
	v.x = -v.x;
	v.y = -v.y;
	v.z = -v.z;
	return v;
}

static inline vec3_t vec3_mul(vec3_t v1, vec3_t v2)
{
	v1.x *= v2.x;
	v1.y *= v2.y;
	v1.z *= v2.z;
	return v1;
}

static inline vec3_t vec3_scale(vec3_t v1, scalar_t s)
{
	v1.x *= s;
	v1.y *= s;
	v1.z *= s;
	return v1;
}

static inline scalar_t vec3_length(vec3_t v)
{
	return (scalar_t)sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline scalar_t vec3_length_sq(vec3_t v)
{
	return v.x * v.x + v.y * v.y + v.z * v.z;
}

static inline vec3_t vec3_normalize(vec3_t v)
{
	scalar_t len = (scalar_t)sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	v.x /= len;
	v.y /= len;
	v.z /= len;
	return v;
}

static inline scalar_t vec3_dot(vec3_t v1, vec3_t v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

static inline vec3_t vec3_cross(vec3_t v1, vec3_t v2)
{
	vec3_t v;
	v.x = v1.y * v2.z - v1.z * v2.y;
	v.y = v1.z * v2.x - v1.x * v2.z;
	v.z = v1.x * v2.y - v1.y * v2.x;
	return v;
}

static inline vec3_t vec3_reflect(vec3_t v, vec3_t n)
{
	vec3_t normal = vec3_normalize(n);
	vec3_t incident = vec3_normalize(v);
	scalar_t val = 2 * vec3_dot(incident, normal);
	return vec3_sub(vec3_scale(normal, val), incident);
}

static inline vec3_t vec3_refract(vec3_t v, vec3_t n, scalar_t ior_src, scalar_t ior_dst)
{
	vec3_t normal = vec3_normalize(n);
	vec3_t incident = vec3_normalize(v);
	scalar_t ior = ior_src / ior_dst;
	scalar_t cos_inc = - vec3_dot(normal, incident);
	scalar_t radical = 1.f - ((ior * ior) * (1.f - (cos_inc * cos_inc)));

	if(radical < 0.f)
	{
		/* total internal reflection */
		return vec3_reflect(v, n);
	}

	scalar_t beta = ior * cos_inc - sqrt(radical);

	return vec3_add( vec3_scale(incident, ior), vec3_scale(normal, beta));
}

static inline void vec3_print(FILE *fp, vec3_t v)
{
	fprintf(fp, "[ %.4f, %.4f, %.4f ]", v.x, v.y, v.z);
}

/* C 4D vector functions */
static inline vec4_t vec4_pack(scalar_t x, scalar_t y, scalar_t z, scalar_t w)
{
	vec4_t v;
	v.x = x;
	v.y = y;
	v.z = z;
	v.w = w;
	return v;
}

static inline vec4_t vec4_add(vec4_t v1, vec4_t v2)
{
	v1.x += v2.x;
	v1.y += v2.y;
	v1.z += v2.z;
	v1.w += v2.w;
	return v1;
}

static inline vec4_t vec4_sub(vec4_t v1, vec4_t v2)
{
	v1.x -= v2.x;
	v1.y -= v2.y;
	v1.z -= v2.z;
	v1.w -= v2.w;
	return v1;
}

static inline vec4_t vec4_neg(vec4_t v)
{
	v.x = -v.x;
	v.y = -v.y;
	v.z = -v.z;
	v.w = -v.w;
	return v;
}

static inline vec4_t vec4_mul(vec4_t v1, vec4_t v2)
{
	v1.x *= v2.x;
	v1.y *= v2.y;
	v1.z *= v2.z;
	v1.w *= v2.w;
	return v1;
}

static inline vec4_t vec4_scale(vec4_t v, scalar_t s)
{
	v.x *= s;
	v.y *= s;
	v.z *= s;
	v.w *= s;
	return v;
}

static inline scalar_t vec4_length(vec4_t v)
{
	return (scalar_t)sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

static inline scalar_t vec4_length_sq(vec4_t v)
{
	return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
}

static inline vec4_t vec4_normalize(vec4_t v)
{
	scalar_t len = (scalar_t)sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
	v.x /= len;
	v.y /= len;
	v.z /= len;
	v.w /= len;
	return v;
}

static inline scalar_t vec4_dot(vec4_t v1, vec4_t v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
}

static inline vec4_t vec4_reflect(vec4_t v, vec4_t n)
{
	vec4_t normal = vec4_normalize(n);
	vec4_t incident = vec4_normalize(v);
	scalar_t val = 2 * vec4_dot(incident, normal);
	return vec4_sub(vec4_scale(normal, val), incident);
}

static inline vec4_t vec4_refract(vec4_t v, vec4_t n, scalar_t ior_src, scalar_t ior_dst)
{
	vec4_t normal = vec4_normalize(n);
	vec4_t incident = vec4_normalize(v);
	scalar_t ior = ior_src / ior_dst;
	scalar_t cos_inc = - vec4_dot(normal, incident);
	scalar_t radical = 1.f - ((ior * ior) * (1.f - (cos_inc * cos_inc)));

	if(radical < 0.f)
	{
		/* total internal reflection */
		return vec4_reflect(v, n);
	}

	scalar_t beta = ior * cos_inc - sqrt(radical);

	return vec4_add( vec4_scale(incident, ior), vec4_scale(normal, beta));
}

static inline void vec4_print(FILE *fp, vec4_t v)
{
	fprintf(fp, "[ %.4f, %.4f, %.4f, %.4f ]", v.x, v.y, v.z, v.w);
}

#ifdef __cplusplus
}	/* extern "C" */

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
    return v;
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
	return (fabs(v1.x - v2.x) < SCALAR_XXSMALL) && (fabs(v1.y - v2.x) < SCALAR_XXSMALL);
}

inline bool operator !=(const Vector2f& v1, const Vector2f& v2)
{
	return (fabs(v1.x - v2.x) >= SCALAR_XXSMALL) && (fabs(v1.y - v2.x) >= SCALAR_XXSMALL);
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
	scalar_t len = length();

	if(!len)
		return;

	x /= len;
	y /= len;
}

inline Vector2f Vector2f::normalized() const
{
	scalar_t len = length();
	return (len != 0) ? Vector2f(x / len, y / len) : *this;
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
    return v;
}

inline const Vector3f operator -(const Vector3f& v)
{
	return Vector3f(-v.x, -v.y, -v.z);
}

inline const Vector3f operator +(const Vector3f& v1, const Vector3f& v2)
{
	return Vector3f(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

inline const Vector3f operator -(const Vector3f& v1, const Vector3f& v2)
{
	return Vector3f(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

inline const Vector3f operator *(const Vector3f& v1, const Vector3f& v2)
{
	return Vector3f(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z);
}

inline const Vector3f operator /(const Vector3f& v1, const Vector3f& v2)
{
	return Vector3f(v1.x / v2.x, v1.y / v2.y, v1.z / v2.z);
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
	v1.x += v2.x;
	v1.y += v2.y;
	v1.z += v2.z;
	return v1;
}

inline Vector3f& operator -=(Vector3f& v1, const Vector3f& v2)
{
	v1.x -= v2.x;
	v1.y -= v2.y;
	v1.z -= v2.z;
	return v1;
}

inline Vector3f& operator *=(Vector3f& v1, const Vector3f& v2)
{
	v1.x *= v2.x;
	v1.y *= v2.y;
	v1.z *= v2.z;
	return v1;
}

inline Vector3f& operator /=(Vector3f& v1, const Vector3f& v2)
{
	v1.x /= v2.x;
	v1.y /= v2.y;
	v1.z /= v2.z;
	return v1;
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
	return (fabs(v1.x - v2.x) >= SCALAR_XXSMALL) && (fabs(v1.y - v2.y) >= SCALAR_XXSMALL) && (fabs(v1.z - v2.z) >= SCALAR_XXSMALL);
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
	return sqrt(x*x + y*y + z*z);
}

inline scalar_t Vector3f::length_squared() const
{
	return x*x + y*y + z*z;
}

inline void Vector3f::normalize()
{
	scalar_t len = length();

	if(!len)
		return;

	x /= len;
	y /= len;
	z /= len;
}

inline Vector3f Vector3f::normalized() const
{
	scalar_t len = length();
	return (len != 0) ? Vector3f(x / len, y / len, z / len) : *this;
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
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

inline Vector3f cross(const Vector3f& v1, const Vector3f& v2)
{
	return Vector3f(v1.y * v2.z - v1.z * v2.y,  v1.z * v2.x - v1.x * v2.z,  v1.x * v2.y - v1.y * v2.x);
}

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
    return v;
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
	return (fabs(v1.x - v2.x) >= SCALAR_XXSMALL) && (fabs(v1.y - v2.y) >= SCALAR_XXSMALL) && (fabs(v1.z - v2.z) >= SCALAR_XXSMALL) && (fabs(v1.w - v2.w) >= SCALAR_XXSMALL);
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
	scalar_t len = length();

	if(!len)
		return;

	x /= len;
	y /= len;
	z /= len;
	w /= len;
}

inline Vector4f Vector4f::normalized() const
{
	scalar_t len = length();
	return (len != 0) ? Vector4f(x / len, y / len, z / len, w / len) : *this;
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

#endif /* __cplusplus */

} /* namespace NMath */

#endif /* NMATH_VECTOR_INL_INCLUDED */
