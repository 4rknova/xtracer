/*

    This file is part of the nemesis math library.

    matrix.inl
    Matrix inline functions

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

#ifndef LIBNMATH_MATRIX_INL_INCLUDED
#define LIBNMATH_MATRIX_INL_INCLUDED

#ifndef LIBNMATH_MATRIX_H_INCLUDED
    #error "matrix.h must be included before matrix.inl"
#endif /* LIBNMATH_MATRIX_H_INCLUDED */

#ifdef __cplusplus
	#include <cstring>
#else
	#include <string.h>
#endif  /* __cplusplus */


#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

/*
	3x3 matrices
*/

static inline void mat3x3_pack(mat3x3_t m,
	scalar_t m11, scalar_t m12, scalar_t m13,
	scalar_t m21, scalar_t m22, scalar_t m23,
	scalar_t m31, scalar_t m32, scalar_t m33)
{
	m[0][0] = m11; m[0][1] = m12; m[0][2] = m13;
	m[1][0] = m21; m[1][1] = m22; m[1][2] = m23;
	m[2][0] = m31; m[2][1] = m32; m[2][2] = m33;
}

static inline void mat3x3_identity(mat3x3_t m)
{
	static const mat3x3_t id = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
	memcpy(m, id, sizeof id);
}

static inline void mat3x3_copy(mat3x3_t dest, mat3x3_t src)
{
	memcpy(dest, src, sizeof(mat3x3_t));
}

static inline void mat3x3_set_column(mat3x3_t m, vec3_t v, int idx)
{
	m[0][idx] = v.x;
	m[1][idx] = v.y;
	m[2][idx] = v.z;
}

static inline void mat3x3_set_row(mat3x3_t m, vec3_t v, int idx)
{
	m[idx][0] = v.x;
	m[idx][1] = v.y;
	m[idx][2] = v.z;
}

static inline void mat3x3_add(mat3x3_t res, mat3x3_t m1, mat3x3_t m2)
{
	int i, j;

	for(i=0; i<3; i++) {
		for(j=0; j<3; j++) {
			res[i][j] = m1[i][j] + m2[i][j];
		}
	}
}

static inline void mat3x3_mul(mat3x3_t res, mat3x3_t m1, mat3x3_t m2)
{
	res[0][0] = m1[0][0] * m2[0][0] + m1[0][1] * m2[1][0] + m1[0][2] * m2[2][0];
	res[0][1] = m1[0][0] * m2[0][1] + m1[0][1] * m2[1][1] + m1[0][2] * m2[2][1];
	res[0][2] = m1[0][0] * m2[0][2] + m1[0][1] * m2[1][2] + m1[0][2] * m2[2][2];

	res[1][0] = m1[1][0] * m2[0][0] + m1[1][1] * m2[1][0] + m1[1][2] * m2[2][0];
	res[1][1] = m1[1][0] * m2[0][1] + m1[1][1] * m2[1][1] + m1[1][2] * m2[2][1];
	res[1][2] = m1[1][0] * m2[0][2] + m1[1][1] * m2[1][2] + m1[1][2] * m2[2][2];

	res[2][0] = m1[2][0] * m2[0][0] + m1[2][1] * m2[1][0] + m1[2][2] * m2[2][0];
	res[2][1] = m1[2][0] * m2[0][1] + m1[2][1] * m2[1][1] + m1[2][2] * m2[2][1];
	res[2][2] = m1[2][0] * m2[0][2] + m1[2][1] * m2[1][2] + m1[2][2] * m2[2][2];
}

/*
	4x4 matrices
*/

static inline void mat4x4_pack(mat4x4_t m,
	scalar_t m11, scalar_t m12, scalar_t m13, scalar_t m14,
	scalar_t m21, scalar_t m22, scalar_t m23, scalar_t m24,
	scalar_t m31, scalar_t m32, scalar_t m33, scalar_t m34,
	scalar_t m41, scalar_t m42, scalar_t m43, scalar_t m44)
{
	m[0][0] = m11; m[0][1] = m12; m[0][2] = m13; m[0][3] = m14;
	m[1][0] = m21; m[1][1] = m22; m[1][2] = m23; m[1][3] = m24;
	m[2][0] = m31; m[2][1] = m32; m[2][2] = m33; m[2][3] = m34;
	m[3][0] = m41; m[3][1] = m42; m[3][2] = m43; m[3][3] = m44;
}

static inline void mat4x4_identity(mat4x4_t m)
{
	static const mat4x4_t id = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
	memcpy(m, id, sizeof id);
}

static inline void mat4x4_copy(mat4x4_t dest, mat4x4_t src)
{
	memcpy(dest, src, sizeof(mat4x4_t));
}

static inline void mat4x4_set_column(mat4x4_t m, vec4_t v, int idx)
{
	m[0][idx] = v.x;
	m[1][idx] = v.y;
	m[2][idx] = v.z;
	m[3][idx] = v.w;
}

static inline void mat4x4_set_row(mat4x4_t m, vec4_t v, int idx)
{
	m[idx][0] = v.x;
	m[idx][1] = v.y;
	m[idx][2] = v.z;
	m[idx][3] = v.w;
}

static inline void mat4x4_add(mat4x4_t res, mat4x4_t m1, mat4x4_t m2)
{
	int i, j;

	for(i=0; i<4; i++) {
		for(j=0; j<4; j++) {
			res[i][j] = m1[i][j] + m2[i][j];
		}
	}
}

static inline void mat4x4_mul(mat4x4_t res, mat4x4_t m1, mat4x4_t m2)
{
	res[0][0] = m1[0][0] * m2[0][0] + m1[0][1] * m2[1][0] + m1[0][2] * m2[2][0] + m1[0][3] * m2[3][0];
	res[0][1] = m1[0][0] * m2[0][1] + m1[0][1] * m2[1][1] + m1[0][2] * m2[2][1] + m1[0][3] * m2[3][1];
	res[0][2] = m1[0][0] * m2[0][2] + m1[0][1] * m2[1][2] + m1[0][2] * m2[2][2] + m1[0][3] * m2[3][2];
	res[0][3] = m1[0][0] * m2[0][3] + m1[0][1] * m2[1][3] + m1[0][2] * m2[2][3] + m1[0][3] * m2[3][3];

	res[1][0] = m1[1][0] * m2[0][0] + m1[1][1] * m2[1][0] + m1[1][2] * m2[2][0] + m1[1][3] * m2[3][0];
	res[1][1] = m1[1][0] * m2[0][1] + m1[1][1] * m2[1][1] + m1[1][2] * m2[2][1] + m1[1][3] * m2[3][1];
	res[1][2] = m1[1][0] * m2[0][2] + m1[1][1] * m2[1][2] + m1[1][2] * m2[2][2] + m1[1][3] * m2[3][2];
	res[1][3] = m1[1][0] * m2[0][3] + m1[1][1] * m2[1][3] + m1[1][2] * m2[2][3] + m1[1][3] * m2[3][3];

	res[2][0] = m1[2][0] * m2[0][0] + m1[2][1] * m2[1][0] + m1[2][2] * m2[2][0] + m1[2][3] * m2[3][0];
	res[2][1] = m1[2][0] * m2[0][1] + m1[2][1] * m2[1][1] + m1[2][2] * m2[2][1] + m1[2][3] * m2[3][1];
	res[2][2] = m1[2][0] * m2[0][2] + m1[2][1] * m2[1][2] + m1[2][2] * m2[2][2] + m1[2][3] * m2[3][2];
	res[2][3] = m1[2][0] * m2[0][3] + m1[2][1] * m2[1][3] + m1[2][2] * m2[2][3] + m1[2][3] * m2[3][3];

	res[3][0] = m1[3][0] * m2[0][0] + m1[3][1] * m2[1][0] + m1[3][2] * m2[2][0] + m1[3][3] * m2[3][0];
	res[3][1] = m1[3][0] * m2[0][1] + m1[3][1] * m2[1][1] + m1[3][2] * m2[2][1] + m1[3][3] * m2[3][1];
	res[3][2] = m1[3][0] * m2[0][2] + m1[3][1] * m2[1][2] + m1[3][2] * m2[2][2] + m1[3][3] * m2[3][2];
	res[3][3] = m1[3][0] * m2[0][3] + m1[3][1] * m2[1][3] + m1[3][2] * m2[2][3] + m1[3][3] * m2[3][3];
}

#ifdef __cplusplus
}   /* extern "C" */

inline scalar_t *Matrix3x3::operator [](int index)
{
    return data[index < 9 ? index : 8];
}

inline const scalar_t *Matrix3x3::operator [](int index) const
{
    return data[index < 9 ? index : 8];
}

inline void Matrix3x3::reset_identity()
{
    memcpy(data, identity.data, 9 * sizeof(scalar_t));
}

inline scalar_t *Matrix4x4::operator [](int index) 
{
    return data[index < 16 ? index : 15];
}

inline const scalar_t *Matrix4x4::operator [](int index) const
{
    return data[index < 16 ? index : 15];
}

inline void Matrix4x4::reset_identity()
{
    memcpy(data, identity.data, 16 * sizeof(scalar_t));
}

#endif

#endif /* LIBNMATH_MATRIX_INL_INCLUDED */
