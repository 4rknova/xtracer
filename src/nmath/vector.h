/*

    This file is part of the nemesis math library.

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

#ifndef LIBNMATH_VECTOR_H_INCLUDED
#define LIBNMATH_VECTOR_H_INCLUDED

#include "types.h"

#ifdef __cplusplus
    #include <cstdio>
#else
    #include <stdio.h>
#endif  /* __cplusplus */

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

#include <ostream>

/*
    2D VECTOR
*/
class Vector2
{
    public:
        /* Constructors */
        Vector2(scalar_t aX = 0.0, scalar_t aY = 0.0);
        Vector2(const Vector2 &v);
        Vector2(const Vector3 &v);
        Vector2(const Vector4 &v);

        /* Array subscript */
        inline scalar_t& operator [](unsigned int index);
        inline const scalar_t& operator [](unsigned int index) const;

        /* Assignment operator */
        inline const Vector2 &operator =(const Vector2 &v);

        /* Unary operator */
        friend inline const Vector2 operator -(const Vector2 &v);

        /* Arithmetic operators */
        /* - Binary */
        friend inline const Vector2 operator +(const Vector2 &v1, const Vector2 &v2);
        friend inline const Vector2 operator -(const Vector2 &v1, const Vector2 &v2);
        friend inline const Vector2 operator *(const Vector2 &v1, const Vector2 &v2);
        friend inline const Vector2 operator /(const Vector2 &v1, const Vector2 &v2);
        /* - Scalar */
        friend inline const Vector2 operator +(scalar_t r, const Vector2 &v);
        friend inline const Vector2 operator +(const Vector2 &v, scalar_t r);
        friend inline const Vector2 operator -(const Vector2 &v, scalar_t r);
        friend inline const Vector2 operator *(scalar_t r, const Vector2 &v);
        friend inline const Vector2 operator *(const Vector2 &v, scalar_t r);
        friend inline const Vector2 operator /(const Vector2 &v, scalar_t r);

        /* Compound assignment operators */
        /* - Binary */
        friend inline Vector2 &operator +=(Vector2 &v1, const Vector2 &v2);
        friend inline Vector2 &operator -=(Vector2 &v1, const Vector2 &v2);
        friend inline Vector2 &operator *=(Vector2 &v1, const Vector2 &v2);
        friend inline Vector2 &operator /=(Vector2 &v1, const Vector2 &v2);
        /* - Scalar */
        friend inline Vector2 &operator +=(Vector2 &vec, scalar_t scalar);
        friend inline Vector2 &operator -=(Vector2 &vec, scalar_t scalar);
        friend inline Vector2 &operator *=(Vector2 &vec, scalar_t scalar);
        friend inline Vector2 &operator /=(Vector2 &vec, scalar_t scalar);

        /* Comparison operations */
        friend inline bool operator ==(const Vector2 &v1, const Vector2 &v2);
        friend inline bool operator !=(const Vector2 &v1, const Vector2 &v2);

        /* Stream operations */
        friend std::ostream& operator <<(std::ostream& out, const Vector2 &vec);

        /* Vector member functions */
        /* - Length */
        inline scalar_t length() const;
        inline scalar_t length_squared() const;
        /* - Normalization */
        inline void normalize();
        inline Vector2 normalized() const;
        /* - Reflection / Refraction */
        inline void reflect(const Vector2 &normal);
        inline Vector2 reflected(const Vector2 &normal) const;
        inline void refract(const Vector2 &normal, scalar_t ior_src, scalar_t ior_dst);
        inline Vector2 refracted(const Vector2 &normal, scalar_t ior_src, scalar_t ior_dst) const;

		/* Transformation */
		inline Vector2 transform(Matrix3x3 &m);
		inline Vector2 transformed(Matrix3x3 &m);

        scalar_t x, y;
};

inline scalar_t dot(const Vector2 &v1, const Vector2 &v2);

/*
    3D VECTOR
*/
class Vector3
{
    public:
        /* Constructors */
        Vector3(scalar_t aX = 0.0, scalar_t aY = 0.0, scalar_t aZ = 0.0);
        Vector3(const Vector3 &v);
        Vector3(const Vector2 &v);
        Vector3(const Vector4 &v);

        /* Array subscript */
        inline scalar_t& operator [](unsigned int index);
        inline const scalar_t& operator [](unsigned int index) const;

        /* Assignment operator */
        inline const Vector3 &operator =(const Vector3 &v);

        /* Unary operator */
        friend inline const Vector3 operator -(const Vector3 &v);

        /* Arithmetic operators */
        /* - Binary */
        friend inline const Vector3 operator +(const Vector3 &v1, const Vector3 &v2);
        friend inline const Vector3 operator -(const Vector3 &v1, const Vector3 &v2);
        friend inline const Vector3 operator *(const Vector3 &v1, const Vector3 &v2);
        friend inline const Vector3 operator /(const Vector3 &v1, const Vector3 &v2);
        /* - Scalar */
        friend inline const Vector3 operator +(scalar_t r, const Vector3 &v);
        friend inline const Vector3 operator +(const Vector3 &v, scalar_t r);
        friend inline const Vector3 operator -(const Vector3 &v, scalar_t r);
        friend inline const Vector3 operator *(scalar_t r, const Vector3 &v);
        friend inline const Vector3 operator *(const Vector3 &v, scalar_t r);
        friend inline const Vector3 operator /(const Vector3 &v, scalar_t r);

        /* Compound assignment operators */
        /* - Binary */
        friend inline Vector3 &operator +=(Vector3 &v1, const Vector3 &v2);
        friend inline Vector3 &operator -=(Vector3 &v1, const Vector3 &v2);
        friend inline Vector3 &operator *=(Vector3 &v1, const Vector3 &v2);
        friend inline Vector3 &operator /=(Vector3 &v1, const Vector3 &v2);
        /* - Scalar */
        friend inline Vector3 &operator +=(Vector3 &vec, scalar_t scalar);
        friend inline Vector3 &operator -=(Vector3 &vec, scalar_t scalar);
        friend inline Vector3 &operator *=(Vector3 &vec, scalar_t scalar);
        friend inline Vector3 &operator /=(Vector3 &vec, scalar_t scalar);

        /* Comparison operations */
        friend inline bool operator ==(const Vector3 &v1, const Vector3 &v2);
        friend inline bool operator !=(const Vector3 &v1, const Vector3 &v2);

        /* Stream operations */
        friend std::ostream& operator <<(std::ostream& out, const Vector3 &vec);

        /* Vector member functions */
        /* - Length */
        inline scalar_t length() const;
        inline scalar_t length_squared() const;
        /* - Normalization */
        inline void normalize();
        inline Vector3 normalized() const;
        /* - Reflection / Refraction */
        inline void reflect(const Vector3 &normal);
        inline Vector3 reflected(const Vector3 &normal) const;
        inline void refract(const Vector3 &normal, scalar_t ior_src, scalar_t ior_dst);
        inline Vector3 refracted(const Vector3 &normal, scalar_t ior_src, scalar_t ior_dst) const;

		/* Transformation */
		inline Vector3 transform(Matrix4x4 &m);
		inline Vector3 transformed(Matrix4x4 &m);

        scalar_t x, y, z;
};

inline scalar_t dot(const Vector3 &v1, const Vector3 &v2);
inline Vector3 cross(const Vector3 &v1, const Vector3 &v2);

/*
    4D VECTOR
*/
class Vector4
{
    public:
        /* Constructors */
        Vector4(scalar_t aX = 0.0, scalar_t aY = 0.0, scalar_t aZ = 0.0, scalar_t aW = 0.0);
        Vector4(const Vector4 &v);
        Vector4(const Vector2 &v);
        Vector4(const Vector3 &v);

        /* Array subscript */
        inline scalar_t& operator [](unsigned int index);
        inline const scalar_t& operator [](unsigned int index) const;

        /* Assignment operator */
        inline const Vector4 &operator =(const Vector4 &v);

        /* Unary operator */
        friend inline const Vector4 operator -(const Vector4 &v);

        /* Arithmetic operators */
        /* - Binary */
        friend inline const Vector4 operator +(const Vector4 &v1, const Vector4 &v2);
        friend inline const Vector4 operator -(const Vector4 &v1, const Vector4 &v2);
        friend inline const Vector4 operator *(const Vector4 &v1, const Vector4 &v2);
        friend inline const Vector4 operator /(const Vector4 &v1, const Vector4 &v2);
        /* - Scalar */
        friend inline const Vector4 operator +(scalar_t r, const Vector4 &v);
        friend inline const Vector4 operator +(const Vector4 &v, scalar_t r);
        friend inline const Vector4 operator -(const Vector4 &v, scalar_t r);
        friend inline const Vector4 operator *(scalar_t r, const Vector4 &v);
        friend inline const Vector4 operator *(const Vector4 &v, scalar_t r);
        friend inline const Vector4 operator /(const Vector4 &v, scalar_t r);

        /* Compound assignment operators */
        /* - Binary */
        friend inline Vector4 &operator +=(Vector4 &v1, const Vector4 &v2);
        friend inline Vector4 &operator -=(Vector4 &v1, const Vector4 &v2);
        friend inline Vector4 &operator *=(Vector4 &v1, const Vector4 &v2);
        friend inline Vector4 &operator /=(Vector4 &v1, const Vector4 &v2);
        /* - Scalar */
        friend inline Vector4 &operator +=(Vector4 &vec, scalar_t scalar);
        friend inline Vector4 &operator -=(Vector4 &vec, scalar_t scalar);
        friend inline Vector4 &operator *=(Vector4 &vec, scalar_t scalar);
        friend inline Vector4 &operator /=(Vector4 &vec, scalar_t scalar);

        /* Comparison operations */
        friend inline bool operator ==(const Vector4 &v1, const Vector4 &v2);
        friend inline bool operator !=(const Vector4 &v1, const Vector4 &v2);

		friend inline bool operator < (const Vector3 &v1, const Vector3 &v2);
		friend inline bool operator > (const Vector3 &v1, const Vector3 &v2);

        /* Stream operations */
        friend std::ostream& operator <<(std::ostream& out, const Vector4 &vec);

        /* Vector member functions */
        /* - Length */
        inline scalar_t length() const;
        inline scalar_t length_squared() const;
        /* - Normalization */
        inline void normalize();
        inline Vector4 normalized() const;
        /* - Reflection / Refraction */
        inline void reflect(const Vector4 &normal);
        inline Vector4 reflected(const Vector4 &normal) const;
        inline void refract(const Vector4 &normal, scalar_t ior_src, scalar_t ior_dst);
        inline Vector4 refracted(const Vector4 &normal, scalar_t ior_src, scalar_t ior_dst) const;

        scalar_t x, y, z, w;
};

inline scalar_t dot(const Vector4 &v1, const Vector4 &v2);

#endif	/* __cplusplus */

#include "vector.inl"

#endif /* LIBNMATH_VECTOR_H_INCLUDED */
