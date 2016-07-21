/*

    This file is part of the libnmath.

    vector.h
    Vector

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

#ifndef NMATH_VECTOR_H_INCLUDED
#define NMATH_VECTOR_H_INCLUDED

#include "defs.h"
#include "types.h"

#ifdef __cplusplus
	#include <ostream>
    #include <cstdio>
#else
    #include <stdio.h>
#endif  /* __cplusplus */

namespace NMath {

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

/*
    C 2D vector functions
*/
static inline vec2_t vec2_pack(scalar_t x, scalar_t y);

static inline vec2_t vec2_add(vec2_t v1, vec2_t v2);
static inline vec2_t vec2_sub(vec2_t v1, vec2_t v2);
static inline vec2_t vec2_neg(vec2_t v);
static inline vec2_t vec2_mul(vec2_t v1, vec2_t v2);
static inline vec2_t vec2_scale(vec2_t v, scalar_t s);

static inline scalar_t vec2_length(vec2_t v);
static inline scalar_t vec2_length_sq(vec2_t v);
static inline vec2_t vec2_normalize(vec2_t v);

static inline scalar_t vec2_dot(vec2_t v1, vec2_t v2);

static inline vec2_t vec2_reflect(vec2_t v, vec2_t n);
static inline vec2_t vec2_refract(vec2_t v, vec2_t n, scalar_t ior_src, scalar_t ior_dst);

static inline void vec2_print(FILE *fp, vec2_t v);

/*
    C 3D vector functions
*/
static inline vec3_t vec3_pack(scalar_t x, scalar_t y, scalar_t z);

static inline vec3_t vec3_add(vec3_t v1, vec3_t v2);
static inline vec3_t vec3_sub(vec3_t v1, vec3_t v2);
static inline vec3_t vec3_neg(vec3_t v);
static inline vec3_t vec3_mul(vec3_t v1, vec3_t v2);
static inline vec3_t vec3_scale(vec3_t v1, scalar_t s);

static inline scalar_t vec3_length(vec3_t v);
static inline scalar_t vec3_length_sq(vec3_t v);
static inline vec3_t vec3_normalize(vec3_t v);

static inline scalar_t vec3_dot(vec3_t v1, vec3_t v2);
static inline vec3_t vec3_cross(vec3_t v1, vec3_t v2);

static inline vec3_t vec3_reflect(vec3_t v, vec3_t n);
static inline vec3_t vec3_refract(vec3_t v, vec3_t n, scalar_t ior_src, scalar_t ior_dst);

static inline void vec3_print(FILE *fp, vec3_t v);

/*
    C 4D vector functions
*/
static inline vec4_t vec4_pack(scalar_t x, scalar_t y, scalar_t z, scalar_t w);

static inline vec4_t vec4_add(vec4_t v1, vec4_t v2);
static inline vec4_t vec4_sub(vec4_t v1, vec4_t v2);
static inline vec4_t vec4_neg(vec4_t v);
static inline vec4_t vec4_mul(vec4_t v1, vec4_t v2);
static inline vec4_t vec4_scale(vec4_t v, scalar_t s);

static inline scalar_t vec4_length(vec4_t v);
static inline scalar_t vec4_length_sq(vec4_t v);
static inline vec4_t vec4_normalize(vec4_t v);

static inline scalar_t vec4_dot(vec4_t v1, vec4_t v2);

static inline vec4_t vec4_reflect(vec4_t v, vec4_t n);
static inline vec4_t vec4_refract(vec4_t v, vec4_t n, scalar_t ior_src, scalar_t ior_dst);

#ifdef __cplusplus
}   /* extern "C" */

/*
    2D VECTOR
*/
class Vector2f
{
    public:
        /* Constructors */
        Vector2f(scalar_t aX = 0.0, scalar_t aY = 0.0);
        Vector2f(const Vector2f &v);
        Vector2f(const Vector3f &v);
        Vector2f(const Vector4f &v);

        /* Array subscript */
        inline scalar_t& operator [](unsigned int index);
        inline const scalar_t& operator [](unsigned int index) const;

        /* Assignment operator */
        inline const Vector2f &operator =(const Vector2f &v);

        /* Unary operator */
        friend inline const Vector2f operator -(const Vector2f &v);

        /* Arithmetic operators */
        /* - Binary */
        friend inline const Vector2f operator +(const Vector2f &v1, const Vector2f &v2);
        friend inline const Vector2f operator -(const Vector2f &v1, const Vector2f &v2);
        friend inline const Vector2f operator *(const Vector2f &v1, const Vector2f &v2);
        friend inline const Vector2f operator /(const Vector2f &v1, const Vector2f &v2);
        /* - Scalar */
        friend inline const Vector2f operator +(scalar_t r, const Vector2f &v);
        friend inline const Vector2f operator +(const Vector2f &v, scalar_t r);
        friend inline const Vector2f operator -(const Vector2f &v, scalar_t r);
        friend inline const Vector2f operator *(scalar_t r, const Vector2f &v);
        friend inline const Vector2f operator *(const Vector2f &v, scalar_t r);
        friend inline const Vector2f operator /(const Vector2f &v, scalar_t r);

        /* Compound assignment operators */
        /* - Binary */
        friend inline Vector2f &operator +=(Vector2f &v1, const Vector2f &v2);
        friend inline Vector2f &operator -=(Vector2f &v1, const Vector2f &v2);
        friend inline Vector2f &operator *=(Vector2f &v1, const Vector2f &v2);
        friend inline Vector2f &operator /=(Vector2f &v1, const Vector2f &v2);
        /* - Scalar */
        friend inline Vector2f &operator +=(Vector2f &vec, scalar_t scalar);
        friend inline Vector2f &operator -=(Vector2f &vec, scalar_t scalar);
        friend inline Vector2f &operator *=(Vector2f &vec, scalar_t scalar);
        friend inline Vector2f &operator /=(Vector2f &vec, scalar_t scalar);

        /* Comparison operations */
        friend inline bool operator ==(const Vector2f &v1, const Vector2f &v2);
        friend inline bool operator !=(const Vector2f &v1, const Vector2f &v2);

        /* Stream operations */
        friend std::ostream& operator <<(std::ostream& out, const Vector2f &vec);

        /* Vector member functions */
        /* - Length */
        inline scalar_t length() const;
        inline scalar_t length_squared() const;
        /* - Normalization */
        inline void normalize();
        inline Vector2f normalized() const;
        /* - Reflection / Refraction */
        inline void reflect(const Vector2f &normal);
        inline Vector2f reflected(const Vector2f &normal) const;
        inline void refract(const Vector2f &normal, scalar_t ior_src, scalar_t ior_dst);
        inline Vector2f refracted(const Vector2f &normal, scalar_t ior_src, scalar_t ior_dst) const;

		/* Transformation */
		inline Vector2f transform(Matrix3x3f &m);
		inline Vector2f transformed(Matrix3x3f &m);

        scalar_t x, y;
};

inline scalar_t dot(const Vector2f &v1, const Vector2f &v2);

/*
    3D VECTOR
*/
class Vector3f
{
    public:
        /* Constructors */
        Vector3f(scalar_t aX = 0.0, scalar_t aY = 0.0, scalar_t aZ = 0.0);
        Vector3f(const Vector3f &v);
        Vector3f(const Vector2f &v);
        Vector3f(const Vector4f &v);

        /* Array subscript */
        inline scalar_t& operator [](unsigned int index);
        inline const scalar_t& operator [](unsigned int index) const;

        /* Assignment operator */
        inline const Vector3f &operator =(const Vector3f &v);

        /* Unary operator */
        friend inline const Vector3f operator -(const Vector3f &v);

        /* Arithmetic operators */
        /* - Binary */
        friend inline const Vector3f operator +(const Vector3f &v1, const Vector3f &v2);
        friend inline const Vector3f operator -(const Vector3f &v1, const Vector3f &v2);
        friend inline const Vector3f operator *(const Vector3f &v1, const Vector3f &v2);
        friend inline const Vector3f operator /(const Vector3f &v1, const Vector3f &v2);
        /* - Scalar */
        friend inline const Vector3f operator +(scalar_t r, const Vector3f &v);
        friend inline const Vector3f operator +(const Vector3f &v, scalar_t r);
        friend inline const Vector3f operator -(const Vector3f &v, scalar_t r);
        friend inline const Vector3f operator *(scalar_t r, const Vector3f &v);
        friend inline const Vector3f operator *(const Vector3f &v, scalar_t r);
        friend inline const Vector3f operator /(const Vector3f &v, scalar_t r);

        /* Compound assignment operators */
        /* - Binary */
        friend inline Vector3f &operator +=(Vector3f &v1, const Vector3f &v2);
        friend inline Vector3f &operator -=(Vector3f &v1, const Vector3f &v2);
        friend inline Vector3f &operator *=(Vector3f &v1, const Vector3f &v2);
        friend inline Vector3f &operator /=(Vector3f &v1, const Vector3f &v2);
        /* - Scalar */
        friend inline Vector3f &operator +=(Vector3f &vec, scalar_t scalar);
        friend inline Vector3f &operator -=(Vector3f &vec, scalar_t scalar);
        friend inline Vector3f &operator *=(Vector3f &vec, scalar_t scalar);
        friend inline Vector3f &operator /=(Vector3f &vec, scalar_t scalar);

        /* Comparison operations */
        friend inline bool operator ==(const Vector3f &v1, const Vector3f &v2);
        friend inline bool operator !=(const Vector3f &v1, const Vector3f &v2);

        /* Stream operations */
        friend std::ostream& operator <<(std::ostream& out, const Vector3f &vec);

        /* Vector member functions */
        /* - Length */
        inline scalar_t length() const;
        inline scalar_t length_squared() const;
        /* - Normalization */
        inline void normalize();
        inline Vector3f normalized() const;
        /* - Reflection / Refraction */
        inline void reflect(const Vector3f &normal);
        inline Vector3f reflected(const Vector3f &normal) const;
        inline void refract(const Vector3f &normal, scalar_t ior_src, scalar_t ior_dst);
        inline Vector3f refracted(const Vector3f &normal, scalar_t ior_src, scalar_t ior_dst) const;

		/* Transformation */
		inline Vector3f transform(Matrix3x3f &m);
		inline Vector3f transformed(Matrix3x3f &m);
		inline Vector3f transform(Matrix4x4f &m);
		inline Vector3f transformed(Matrix4x4f &m);

        scalar_t x, y, z;
};

inline scalar_t dot(const Vector3f &v1, const Vector3f &v2);
inline Vector3f cross(const Vector3f &v1, const Vector3f &v2);

/*
    4D VECTOR
*/
class Vector4f
{
    public:
        /* Constructors */
        Vector4f(scalar_t aX = 0.0, scalar_t aY = 0.0, scalar_t aZ = 0.0, scalar_t aW = 0.0);
        Vector4f(const Vector4f &v);
        Vector4f(const Vector2f &v);
        Vector4f(const Vector3f &v);

        /* Array subscript */
        inline scalar_t& operator [](unsigned int index);
        inline const scalar_t& operator [](unsigned int index) const;

        /* Assignment operator */
        inline const Vector4f &operator =(const Vector4f &v);

        /* Unary operator */
        friend inline const Vector4f operator -(const Vector4f &v);

        /* Arithmetic operators */
        /* - Binary */
        friend inline const Vector4f operator +(const Vector4f &v1, const Vector4f &v2);
        friend inline const Vector4f operator -(const Vector4f &v1, const Vector4f &v2);
        friend inline const Vector4f operator *(const Vector4f &v1, const Vector4f &v2);
        friend inline const Vector4f operator /(const Vector4f &v1, const Vector4f &v2);
        /* - Scalar */
        friend inline const Vector4f operator +(scalar_t r, const Vector4f &v);
        friend inline const Vector4f operator +(const Vector4f &v, scalar_t r);
        friend inline const Vector4f operator -(const Vector4f &v, scalar_t r);
        friend inline const Vector4f operator *(scalar_t r, const Vector4f &v);
        friend inline const Vector4f operator *(const Vector4f &v, scalar_t r);
        friend inline const Vector4f operator /(const Vector4f &v, scalar_t r);

        /* Compound assignment operators */
        /* - Binary */
        friend inline Vector4f &operator +=(Vector4f &v1, const Vector4f &v2);
        friend inline Vector4f &operator -=(Vector4f &v1, const Vector4f &v2);
        friend inline Vector4f &operator *=(Vector4f &v1, const Vector4f &v2);
        friend inline Vector4f &operator /=(Vector4f &v1, const Vector4f &v2);
        /* - Scalar */
        friend inline Vector4f &operator +=(Vector4f &vec, scalar_t scalar);
        friend inline Vector4f &operator -=(Vector4f &vec, scalar_t scalar);
        friend inline Vector4f &operator *=(Vector4f &vec, scalar_t scalar);
        friend inline Vector4f &operator /=(Vector4f &vec, scalar_t scalar);

        /* Comparison operations */
        friend inline bool operator ==(const Vector4f &v1, const Vector4f &v2);
        friend inline bool operator !=(const Vector4f &v1, const Vector4f &v2);

		friend inline bool operator < (const Vector3f &v1, const Vector3f &v2);
		friend inline bool operator > (const Vector3f &v1, const Vector3f &v2);

        /* Stream operations */
        friend std::ostream& operator <<(std::ostream& out, const Vector4f &vec);

        /* Vector member functions */
        /* - Length */
        inline scalar_t length() const;
        inline scalar_t length_squared() const;
        /* - Normalization */
        inline void normalize();
        inline Vector4f normalized() const;
        /* - Reflection / Refraction */
        inline void reflect(const Vector4f &normal);
        inline Vector4f reflected(const Vector4f &normal) const;
        inline void refract(const Vector4f &normal, scalar_t ior_src, scalar_t ior_dst);
        inline Vector4f refracted(const Vector4f &normal, scalar_t ior_src, scalar_t ior_dst) const;

        scalar_t x, y, z, w;
};

inline scalar_t dot(const Vector4f &v1, const Vector4f &v2);

#endif	/* __cplusplus */

} /* namespace NMath */

#include "vector.inl"

#endif /* NMATH_VECTOR_H_INCLUDED */
