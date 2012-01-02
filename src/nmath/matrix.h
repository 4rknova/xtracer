/*

    This file is part of the nemesis math library.

    matrix.h
    Matrix

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

/*
	To do
	-----

	- Mat3x3:
		- Adjoint

	- Mat4x4:
		- Arbitrary axis rotation

	- Quaternions
*/

#ifndef LIBNMATH_MATRIX_H_INCLUDED
#define LIBNMATH_MATRIX_H_INCLUDED

#ifdef __cplusplus
    #include <cstdio>
#else
    #include <stdio.h>
#endif  /* __cplusplus */

#include "precision.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

/*
	3x3 matrices
*/
static inline void mat3x3_pack(mat3x3_t m,
		scalar_t m11, scalar_t m12, scalar_t m13,
		scalar_t m21, scalar_t m22, scalar_t m23,
		scalar_t m31, scalar_t m32, scalar_t m33);

static inline void mat3x3_identity(mat3x3_t m);

static inline void mat3x3_copy(mat3x3_t dest, mat3x3_t src);

static inline void mat3x3_set_column(mat3x3_t m, vec3_t v, int idx);
static inline void mat3x3_set_row(mat3x3_t m, vec3_t v, int idx);

static inline void mat3x3_add(mat3x3_t res, mat3x3_t m1, mat3x3_t m2);
static inline void mat3x3_mul(mat3x3_t res, mat3x3_t m1, mat3x3_t m2);

void mat3x3_translate(mat3x3_t m, scalar_t x, scalar_t y);
void mat3x3_rotate(mat3x3_t m, scalar_t angle);
void mat3x3_scale(mat3x3_t m, scalar_t x, scalar_t y);
void mat3x3_shear(mat3x3_t m, scalar_t s);
void mat3x3_mirror_x(mat3x3_t m);
void mat3x3_mirror_y(mat3x3_t m);

void mat3x3_transpose(mat3x3_t res, mat3x3_t m);
scalar_t mat3x3_determinant(mat3x3_t m);
void mat3x3_adjoint(mat3x3_t res, mat3x3_t m);
void mat3x3_inverse(mat3x3_t res, mat3x3_t m);

void mat3x3_to_m4x4(mat4x4_t dest, mat3x3_t src);

void mat3x3_print(FILE *fp, mat3x3_t m);

/*
	4x4 matrices
*/

static inline void mat4x4_pack(mat4x4_t m,
		scalar_t m11, scalar_t m12, scalar_t m13, scalar_t m14,
		scalar_t m21, scalar_t m22, scalar_t m23, scalar_t m24,
		scalar_t m31, scalar_t m32, scalar_t m33, scalar_t m34,
		scalar_t m41, scalar_t m42, scalar_t m43, scalar_t m44);

static inline void mat4x4_identity(mat4x4_t m);

static inline void mat4x4_copy(mat4x4_t dest, mat4x4_t src);

static inline void mat4x4_set_column(mat4x4_t m, vec4_t v, int idx);
static inline void mat4x4_set_row(mat4x4_t m, vec4_t v, int idx);

static inline void mat4x4_add(mat4x4_t res, mat4x4_t m1, mat4x4_t m2);
static inline void mat4x4_mul(mat4x4_t res, mat4x4_t m1, mat4x4_t m2);

void mat4x4_translate(mat4x4_t m, scalar_t x, scalar_t y, scalar_t z);
void mat4x4_rotate(mat4x4_t m, scalar_t x, scalar_t y, scalar_t z);
void mat4x4_rotate_x(mat4x4_t m, scalar_t angle);
void mat4x4_rotate_y(mat4x4_t m, scalar_t angle);
void mat4x4_rotate_z(mat4x4_t m, scalar_t angle);
void mat4x4_rotate_axis(mat4x4_t m, scalar_t angle, scalar_t x, scalar_t y, scalar_t z);
void mat4x4_scale(mat4x4_t m, scalar_t x, scalar_t y, scalar_t z);

void mat4x4_transpose(mat4x4_t res, mat4x4_t m);
scalar_t mat4x4_determinant(mat4x4_t m);
void mat4x4_adjoint(mat4x4_t res, mat4x4_t m);
void mat4x4_inverse(mat4x4_t res, mat4x4_t m);

void mat4x4_to_m3x3(mat3x3_t dest, mat4x4_t src);

void mat4x4_print(FILE *fp, mat4x4_t m);

#ifdef __cplusplus
}   /* extern "C" */

#include <ostream>

class Matrix3x3
{
	friend class Matrix4x4;
	public:
		/* Constructors */
		Matrix3x3();
		Matrix3x3(	scalar_t m11, scalar_t m12, scalar_t m13,
					scalar_t m21, scalar_t m22, scalar_t m23,
					scalar_t m31, scalar_t m32, scalar_t m33);
		Matrix3x3(const mat3x3_t m);
		Matrix3x3(const Matrix4x4 &mat4);

		/* Binary operators */
		friend Matrix3x3 operator +(const Matrix3x3 &m1, const Matrix3x3 &m2);
		friend Matrix3x3 operator -(const Matrix3x3 &m1, const Matrix3x3 &m2);
		friend Matrix3x3 operator *(const Matrix3x3 &m1, const Matrix3x3 &m2);

		/* Compound binary operators */
		friend void operator +=(Matrix3x3 &m1, const Matrix3x3 &m2);
		friend void operator -=(Matrix3x3 &m1, const Matrix3x3 &m2);
		friend void operator *=(Matrix3x3 &m1, const Matrix3x3 &m2);

		/* Scalar operators */
		friend Matrix3x3 operator *(const Matrix3x3 &mat, scalar_t r);
		friend Matrix3x3 operator *(scalar_t r, const Matrix3x3 &mat);

		/* Vector operators */
		friend Vector3 operator *(const Matrix3x3 &mat, const Vector3 &vec);

		/* Compound scalar operators */
		friend void operator *=(Matrix3x3 &mat, scalar_t r);

		/* Index operator */
		inline scalar_t *operator [](int index);
		inline const scalar_t *operator [](int index) const;

		/* Reset matrix */
		inline void reset_identity();

		/* Transformations */
		void translate(const Vector2 &trans);
		void set_translation(const Vector2 &trans);

		void rotate(scalar_t angle);						/* 2d rotation */
    	void rotate(const Vector3 &euler);				/* 3d rotation with euler angles */
    	void rotate(const Vector3 &axis, scalar_t angle);	/* 3d axis/angle rotation */

		void set_rotation(scalar_t angle);
    	void set_rotation(const Vector3 &euler);
		void set_rotation(const Vector3 &axis, scalar_t angle);

		void scale(const Vector3 &vec);
		void set_scaling(const Vector3 &vec);

		/* Tuple operations */
		void set_column_vector(const Vector3 &vec, unsigned int index);
		void set_row_vector(const Vector3 &vec, unsigned int index);
		Vector3 get_column_vector(unsigned int index) const;
		Vector3 get_row_vector(unsigned int index) const;

		void transpose();
		Matrix3x3 transposed() const;
		scalar_t determinant() const;
		Matrix3x3 adjoint() const;
		Matrix3x3 inverse() const;

	    friend std::ostream &operator <<(std::ostream &out, const Matrix3x3 &mat);

		static const Matrix3x3 identity;

		scalar_t data[3][3];
};

class Matrix4x4
{
	friend class Matrix3x3;
	public:
		/* Constructors */
		Matrix4x4();
		Matrix4x4(  scalar_t m11, scalar_t m12, scalar_t m13, scalar_t m14,
					scalar_t m21, scalar_t m22, scalar_t m23, scalar_t m24,
					scalar_t m31, scalar_t m32, scalar_t m33, scalar_t m34,
					scalar_t m41, scalar_t m42, scalar_t m43, scalar_t m44);
		Matrix4x4(const mat4x4_t m);
		Matrix4x4(const Matrix3x3 &mat3);

		/* Binary operators */
		friend Matrix4x4 operator +(const Matrix4x4 &m1, const Matrix4x4 &m2);
		friend Matrix4x4 operator -(const Matrix4x4 &m1, const Matrix4x4 &m2);
		friend Matrix4x4 operator *(const Matrix4x4 &m1, const Matrix4x4 &m2);

		/* Compound binary operators */
		friend void operator +=(Matrix4x4 &m1, const Matrix4x4 &m2);
		friend void operator -=(Matrix4x4 &m1, const Matrix4x4 &m2);
		friend void operator *=(Matrix4x4 &m1, const Matrix4x4 &m2);

		/* Scalar operators */
		friend Matrix4x4 operator *(const Matrix4x4 &mat, scalar_t r);
		friend Matrix4x4 operator *(scalar_t r, const Matrix4x4 &mat);

		/* Vector operators */
		friend Vector4 operator *(const Matrix4x4 &mat, const Vector4 &vec);

		/* Compound scalar operators */
		friend void operator *=(Matrix4x4 &mat, scalar_t r);

		/* Index operator */
		inline scalar_t *operator [](int index);
		inline const scalar_t *operator [](int index) const;

		/* Reset matrix */
		inline void reset_identity();

		/* Transformations */
		void translate(const Vector3 &trans);
		void set_translation(const Vector3 &trans);

		void rotate(const Vector3 &euler);           	/* 3d rotation with euler angles */
		void rotate(const Vector3 &axis, scalar_t angle); /* 3d axis/angle rotation */
		void set_rotation(const Vector3 &euler);
		void set_rotation(const Vector3 &axis, scalar_t angle);

		void scale(const Vector4 &vec);
		void set_scaling(const Vector4 &vec);
		
		/* Tuple operations */
 		void set_column_vector(const Vector4 &vec, unsigned int index);
		void set_row_vector(const Vector4 &vec, unsigned int index);
		Vector4 get_column_vector(unsigned int index) const;
		Vector4 get_row_vector(unsigned int index) const;

		void transpose();
		Matrix4x4 transposed() const;
		scalar_t determinant() const;
		Matrix4x4 adjoint() const;
		Matrix4x4 inverse() const;

		friend std::ostream &operator <<(std::ostream &out, const Matrix4x4 &mat);

		static const Matrix4x4 identity;

		scalar_t data[4][4];
};

#endif	/* __cplusplus */

#include "matrix.inl"

#endif /* LIBNMATH_MATRIX_H_INCLUDED */
