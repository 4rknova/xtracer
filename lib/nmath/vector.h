#ifndef NMATH_VECTOR_H_INCLUDED
#define NMATH_VECTOR_H_INCLUDED

#include "types.h"

#include <ostream>
#include <cstdio>

namespace nmath {

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

} /* namespace nmath */

#include "vector.inl"

#endif /* NMATH_VECTOR_H_INCLUDED */
