/*

    This file is part of the nemesis math library.

    matrix.cc
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

#include "matrix.h"
#include "vector.h"

#ifdef __cplusplus
    #include <cmath>
	#include <cstdio>
#else
    #include <math.h>
	#include <stdio.h>
#endif  /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

/*
	3x3 matrices
*/

void mat3x3_translate(mat3x3_t m, scalar_t x, scalar_t y)
{
	mat3x3_t tm;
	mat3x3_identity(tm);
	tm[0][3] = x;
	tm[1][3] = y;
	mat3x3_mul(m, m, tm);
}

void mat3x3_rotate(mat3x3_t m, scalar_t angle)
{
	mat3x3_t rm;
	mat3x3_identity(rm);
	rm[0][0] = cos(angle); rm[0][1] = -sin(angle);
	rm[1][0] = sin(angle); rm[1][1] = cos(angle);
	mat3x3_mul(m, m, rm);
}

void mat3x3_scale(mat3x3_t m, scalar_t x, scalar_t y)
{
	mat3x3_t sm;
	mat3x3_identity(sm);
	sm[0][0] = x;
	sm[1][1] = y;
	mat3x3_mul(m, m, sm);
}

void mat3x3_shear(mat3x3_t m, scalar_t s)
{
	mat3x3_t rm;
	mat3x3_identity(rm);
	rm[0][1] = s;
	mat3x3_mul(m, m, rm);
}

void mat3x3_mirror_x(mat3x3_t m)
{
	static mat3x3_t id = {{-1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
	mat3x3_mul(m, m, id);
}

void mat3x3_mirror_y(mat3x3_t m)
{
	static mat3x3_t id = {{1, 0, 0}, {0, -1, 0}, {0, 0, 1}};
	mat3x3_mul(m, m, id);
}

void mat3x3_transpose(mat3x3_t res, mat3x3_t m)
{
	int i, j;
	mat3x3_t tmp;
	mat3x3_copy(tmp, m);

	for(i=0; i<3; i++) {
		for(j=0; j<3; j++) {
			res[i][j] = tmp[j][i];
		}
	}
}

scalar_t mat3x3_determinant(mat3x3_t m)
{
	scalar_t det11 = m[1][1] * (m[2][2] - m[1][2] * m[2][1]);
	scalar_t det12 = m[1][0] * (m[2][2] - m[1][2] * m[2][0]);
	scalar_t det13 = m[1][0] * (m[2][1] - m[1][1] * m[2][0]);

	return m[0][0] * det11 - m[0][1] * det12 + m[0][2] * det13;
}

void mat3x3_adjoint(mat3x3_t res, mat3x3_t m)
{
	mat3x3_t coef;

    coef[0][0] =  (m[1][1] * m[2][2] - m[1][2] * m[2][1]);
    coef[0][1] =  (m[1][0] * m[2][2] - m[1][2] * m[2][0]);
    coef[0][2] =  (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
    coef[1][0] =  (m[0][1] * m[2][2] - m[0][2] * m[2][1]);
    coef[1][1] =  (m[0][0] * m[2][2] - m[0][2] * m[2][0]);
    coef[1][2] =  (m[0][0] * m[2][1] - m[0][1] * m[2][0]);
    coef[2][0] =  (m[0][1] * m[1][2] - m[0][2] * m[1][1]);
    coef[2][1] =  (m[0][0] * m[1][2] - m[0][2] * m[1][0]);
    coef[2][2] =  (m[0][0] * m[1][1] - m[0][1] * m[1][0]);

    mat3x3_transpose(res, coef);

	int i=0, j=0;
    for(i=0; i<3; i++) {
        for(j=0; j<3; j++) {
            res[i][j] = j%2 ? -res[i][j] : res[i][j];
            if(i%2)
                res[i][j] = -res[i][j];
        }
    }
}

void mat3x3_inverse(mat3x3_t res, mat3x3_t m)
{
	int i, j;
	mat3x3_t adj;
	scalar_t det;

	det = mat3x3_determinant(m);

	if(!det){
		return;
	}

	mat3x3_adjoint(adj, m);

	for(i=0; i<3; i++) {
		for(j=0; j<3; j++) {
			res[i][j] = adj[i][j] / det;
		}
	}
}

void mat3x3_to_m4x4(mat4x4_t dest, mat3x3_t src)
{
	int i, j;

	memset(dest, 0, sizeof(mat4x4_t));
	for(i=0; i<3; i++) {
		for(j=0; j<3; j++) {
			dest[i][j] = src[i][j];
		}
	}
	dest[3][3] = 1.0;
}

void mat3x3_print(FILE *fp, mat3x3_t m)
{
	int i;
	for(i=0; i<3; i++) {
		fprintf(fp, "[ %12.5f, %12.5f, %12.5f ]\n", (float)m[i][0], (float)m[i][1], (float)m[i][2]);
	}
}

/*
	4x4 matrices
*/

void mat4x4_translate(mat4x4_t m, scalar_t x, scalar_t y, scalar_t z)
{
	mat4x4_t tm;
	mat4x4_identity(tm);
	tm[0][3] = x;
	tm[1][3] = y;
	tm[2][3] = z;
	mat4x4_mul(m, m, tm);
}

void mat4x4_rotate(mat4x4_t m, scalar_t x, scalar_t y, scalar_t z)
{
	mat4x4_rotate_x(m, x);
	mat4x4_rotate_y(m, y);
	mat4x4_rotate_z(m, z);
}

void mat4x4_rotate_x(mat4x4_t m, scalar_t angle)
{
	mat4x4_t rm;
	mat4x4_identity(rm);
	rm[1][1] = cos(angle); rm[1][2] = -sin(angle);
	rm[2][1] = sin(angle); rm[2][2] = cos(angle);
	mat4x4_mul(m, m, rm);
}

void mat4x4_rotate_y(mat4x4_t m, scalar_t angle)
{
	mat4x4_t rm;
	mat4x4_identity(rm);
	rm[0][0] = cos(angle); rm[0][2] = sin(angle);
	rm[2][0] = -sin(angle); rm[2][2] = cos(angle);
	mat4x4_mul(m, m, rm);
}

void mat4x4_rotate_z(mat4x4_t m, scalar_t angle)
{
	mat4x4_t rm;
	mat4x4_identity(rm);
	rm[0][0] = cos(angle); rm[0][1] = -sin(angle);
	rm[1][0] = sin(angle); rm[1][1] = cos(angle);
	mat4x4_mul(m, m, rm);
}

void mat4x4_rotate_axis(mat4x4_t m, scalar_t angle, scalar_t x, scalar_t y, scalar_t z)
{
	mat4x4_t xform;
	scalar_t sina = sin(angle);
	scalar_t cosa = cos(angle);
	scalar_t one_minus_cosa = 1.0 - cosa;
	scalar_t nxsq = x * x;
	scalar_t nysq = y * y;
	scalar_t nzsq = z * z;

	mat4x4_identity(xform);
	xform[0][0] = nxsq + (1.0 - nxsq) * cosa;
	xform[0][1] = x * y * one_minus_cosa - z * sina;
	xform[0][2] = x * z * one_minus_cosa + y * sina;
	xform[1][0] = x * y * one_minus_cosa + z * sina;
	xform[1][1] = nysq + (1.0 - nysq) * cosa;
	xform[1][2] = y * z * one_minus_cosa - x * sina;
	xform[2][0] = x * z * one_minus_cosa - y * sina;
	xform[2][1] = y * z * one_minus_cosa + x * sina;
	xform[2][2] = nzsq + (1.0 - nzsq) * cosa;

	mat4x4_mul(m, m, xform);
}

void mat4x4_scale(mat4x4_t m, scalar_t x, scalar_t y, scalar_t z)
{
	mat4x4_t sm;
	mat4x4_identity(sm);
	sm[0][0] = x;
	sm[1][1] = y;
	sm[2][2] = z;
	mat4x4_mul(m, m, sm);
}

void mat4x4_transpose(mat4x4_t res, mat4x4_t m)
{
	int i, j;
	mat4x4_t tmp;
	mat4x4_copy(tmp, m);

	for(i=0; i<4; i++) {
		for(j=0; j<4; j++) {
			res[i][j] = tmp[j][i];
		}
	}
}

scalar_t mat4x4_determinant(mat4x4_t m)
{
	scalar_t det11 =	(m[1][1] * (m[2][2] * m[3][3] - m[3][2] * m[2][3])) -
						(m[1][2] * (m[2][1] * m[3][3] - m[3][1] * m[2][3])) +
						(m[1][3] * (m[2][1] * m[3][2] - m[3][1] * m[2][2]));

	scalar_t det12 =	(m[1][0] * (m[2][2] * m[3][3] - m[3][2] * m[2][3])) -
						(m[1][2] * (m[2][0] * m[3][3] - m[3][0] * m[2][3])) +
						(m[1][3] * (m[2][0] * m[3][2] - m[3][0] * m[2][2]));

	scalar_t det13 =	(m[1][0] * (m[2][1] * m[3][3] - m[3][1] * m[2][3])) -
						(m[1][1] * (m[2][0] * m[3][3] - m[3][0] * m[2][3])) +
						(m[1][3] * (m[2][0] * m[3][1] - m[3][0] * m[2][1]));

	scalar_t det14 =	(m[1][0] * (m[2][1] * m[3][2] - m[3][1] * m[2][2])) -
						(m[1][1] * (m[2][0] * m[3][2] - m[3][0] * m[2][2])) +
						(m[1][2] * (m[2][0] * m[3][1] - m[3][0] * m[2][1]));

	return m[0][0] * det11 - m[0][1] * det12 + m[0][2] * det13 - m[0][3] * det14;
}

void mat4x4_adjoint(mat4x4_t res, mat4x4_t m)
{
	int i, j;
	mat4x4_t coef;

	coef[0][0] =	(m[1][1] * (m[2][2] * m[3][3] - m[3][2] * m[2][3])) -
					(m[1][2] * (m[2][1] * m[3][3] - m[3][1] * m[2][3])) +
					(m[1][3] * (m[2][1] * m[3][2] - m[3][1] * m[2][2]));

	coef[0][1] =	(m[1][0] * (m[2][2] * m[3][3] - m[3][2] * m[2][3])) -
					(m[1][2] * (m[2][0] * m[3][3] - m[3][0] * m[2][3])) +
					(m[1][3] * (m[2][0] * m[3][2] - m[3][0] * m[2][2]));

	coef[0][2] =	(m[1][0] * (m[2][1] * m[3][3] - m[3][1] * m[2][3])) -
					(m[1][1] * (m[2][0] * m[3][3] - m[3][0] * m[2][3])) +
					(m[1][3] * (m[2][0] * m[3][1] - m[3][0] * m[2][1]));
	coef[0][3] =	(m[1][0] * (m[2][1] * m[3][2] - m[3][1] * m[2][2])) -
					(m[1][1] * (m[2][0] * m[3][2] - m[3][0] * m[2][2])) +
					(m[1][2] * (m[2][0] * m[3][1] - m[3][0] * m[2][1]));

	coef[1][0] =	(m[0][1] * (m[2][2] * m[3][3] - m[3][2] * m[2][3])) -
					(m[0][2] * (m[2][1] * m[3][3] - m[3][1] * m[2][3])) +
					(m[0][3] * (m[2][1] * m[3][2] - m[3][1] * m[2][2]));
	coef[1][1] =	(m[0][0] * (m[2][2] * m[3][3] - m[3][2] * m[2][3])) -
					(m[0][2] * (m[2][0] * m[3][3] - m[3][0] * m[2][3])) +
					(m[0][3] * (m[2][0] * m[3][2] - m[3][0] * m[2][2]));
	coef[1][2] =	(m[0][0] * (m[2][1] * m[3][3] - m[3][1] * m[2][3])) -
					(m[0][1] * (m[2][0] * m[3][3] - m[3][0] * m[2][3])) +
					(m[0][3] * (m[2][0] * m[3][1] - m[3][0] * m[2][1]));
	coef[1][3] =	(m[0][0] * (m[2][1] * m[3][2] - m[3][1] * m[2][2])) -
					(m[0][1] * (m[2][0] * m[3][2] - m[3][0] * m[2][2])) +
					(m[0][2] * (m[2][0] * m[3][1] - m[3][0] * m[2][1]));

	coef[2][0] =	(m[0][1] * (m[1][2] * m[3][3] - m[3][2] * m[1][3])) -
					(m[0][2] * (m[1][1] * m[3][3] - m[3][1] * m[1][3])) +
					(m[0][3] * (m[1][1] * m[3][2] - m[3][1] * m[1][2]));
	coef[2][1] =	(m[0][0] * (m[1][2] * m[3][3] - m[3][2] * m[1][3])) -
					(m[0][2] * (m[1][0] * m[3][3] - m[3][0] * m[1][3])) +
					(m[0][3] * (m[1][0] * m[3][2] - m[3][0] * m[1][2]));
	coef[2][2] =	(m[0][0] * (m[1][1] * m[3][3] - m[3][1] * m[1][3])) -
					(m[0][1] * (m[1][0] * m[3][3] - m[3][0] * m[1][3])) +
					(m[0][3] * (m[1][0] * m[3][1] - m[3][0] * m[1][1]));
	coef[2][3] =	(m[0][0] * (m[1][1] * m[3][2] - m[3][1] * m[1][2])) -
					(m[0][1] * (m[1][0] * m[3][2] - m[3][0] * m[1][2])) +
					(m[0][2] * (m[1][0] * m[3][1] - m[3][0] * m[1][1]));

	coef[3][0] =	(m[0][1] * (m[1][2] * m[2][3] - m[2][2] * m[1][3])) -
					(m[0][2] * (m[1][1] * m[2][3] - m[2][1] * m[1][3])) +
					(m[0][3] * (m[1][1] * m[2][2] - m[2][1] * m[1][2]));
	coef[3][1] =	(m[0][0] * (m[1][2] * m[2][3] - m[2][2] * m[1][3])) -
					(m[0][2] * (m[1][0] * m[2][3] - m[2][0] * m[1][3])) +
					(m[0][3] * (m[1][0] * m[2][2] - m[2][0] * m[1][2]));
	coef[3][2] =	(m[0][0] * (m[1][1] * m[2][3] - m[2][1] * m[1][3])) -
					(m[0][1] * (m[1][0] * m[2][3] - m[2][0] * m[1][3])) +
					(m[0][3] * (m[1][0] * m[2][1] - m[2][0] * m[1][1]));
	coef[3][3] =	(m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2])) -
					(m[0][1] * (m[1][0] * m[2][2] - m[2][0] * m[1][2])) +
					(m[0][2] * (m[1][0] * m[2][1] - m[2][0] * m[1][1]));

	mat4x4_transpose(res, coef);

	for(i=0; i<4; i++) {
		for(j=0; j<4; j++) {
			res[i][j] = j % 2 ? -res[i][j] : res[i][j];
			if(i % 2) res[i][j] = -res[i][j];
		}
	}
}

void mat4x4_inverse(mat4x4_t res, mat4x4_t m)
{
	int i, j;
	mat4x4_t adj;
	scalar_t det;

	det = mat4x4_determinant(m);

	if(!det){
		return;
	}

	mat4x4_adjoint(adj, m);

	for(i=0; i<4; i++) {
		for(j=0; j<4; j++) {
			res[i][j] = adj[i][j] / det;
		}
	}
}

void mat4x4_to_m3x3(mat3x3_t dest, mat4x4_t src)
{
	int i, j;
	for(i=0; i<3; i++) {
		for(j=0; j<3; j++) {
			dest[i][j] = src[i][j];
		}
	}
}

void mat4x4_print(FILE *fp, mat4x4_t m)
{
	int i;
	for(i=0; i<4; i++) {
		fprintf(fp, "[ %12.5f, %12.5f, %12.5f, %12.5f ]\n", (float)m[i][0], (float)m[i][1], (float)m[i][2], (float)m[i][3]);
	}
}

#ifdef __cplusplus
}   /* extern "C" */

const Matrix3x3 Matrix3x3::identity = Matrix3x3(1, 0, 0, 0, 1, 0, 0, 0, 1);

Matrix3x3::Matrix3x3()
{
    *this = identity;
}

Matrix3x3::Matrix3x3(	scalar_t m11, scalar_t m12, scalar_t m13,
						scalar_t m21, scalar_t m22, scalar_t m23,
						scalar_t m31, scalar_t m32, scalar_t m33)
{
	data[0][0] = m11; data[0][1] = m12; data[0][2] = m13;
	data[1][0] = m21; data[1][1] = m22; data[1][2] = m23;
	data[2][0] = m31; data[2][1] = m32; data[2][2] = m33;
}

Matrix3x3::Matrix3x3(const mat3x3_t m)
{
	memcpy(data, m, sizeof(mat3x3_t));
}

Matrix3x3::Matrix3x3(const Matrix4x4 &mat4)
{
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++) {
            data[i][j] = mat4[i][j];
        }
    }
}

Matrix3x3 operator +(const Matrix3x3 &m1, const Matrix3x3 &m2)
{
	Matrix3x3 res;
	const scalar_t *op1 = m1.data[0], *op2 = m2.data[0];
    scalar_t *dest = res.data[0];

    for(int i=0; i<9; i++) {
        *dest++ = *op1++ + *op2++;
    }
    return res;
}

Matrix3x3 operator -(const Matrix3x3 &m1, const Matrix3x3 &m2)
{
    Matrix3x3 res;
    const scalar_t *op1 = m1.data[0], *op2 = m2.data[0];
    scalar_t *dest = res.data[0];

    for(int i=0; i<9; i++) {
        *dest++ = *op1++ - *op2++;
    }
    return res;
}

Matrix3x3 operator *(const Matrix3x3 &m1, const Matrix3x3 &m2)
{
	Matrix3x3 res;
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++) {
            res.data[i][j] = m1.data[i][0] * m2.data[0][j] + m1.data[i][1] * m2.data[1][j] + m1.data[i][2] * m2.data[2][j];
        }
    }
    return res;
}

void operator +=(Matrix3x3 &m1, const Matrix3x3 &m2)
{
    scalar_t *op1 = m1.data[0];
    const scalar_t *op2 = m2.data[0];

    for(int i=0; i<9; i++) {
        *op1++ += *op2++;
    }
}

void operator -=(Matrix3x3 &m1, const Matrix3x3 &m2)
{
    scalar_t *op1 = m1.data[0];
    const scalar_t *op2 = m2.data[0];

    for(int i=0; i<9; i++) {
        *op1++ -= *op2++;
    }
}

void operator *=(Matrix3x3 &m1, const Matrix3x3 &m2)
{
    Matrix3x3 res;
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++) {
            res.data[i][j] = m1.data[i][0] * m2.data[0][j] + m1.data[i][1] * m2.data[1][j] + m1.data[i][2] * m2.data[2][j];
        }
    }
    memcpy(m1.data, res.data, 9 * sizeof(scalar_t));
}

Matrix3x3 operator *(const Matrix3x3 &mat, scalar_t r)
{
	Matrix3x3 res;
    const scalar_t *mptr = mat.data[0];
    scalar_t *dptr = res.data[0];

    for(int i=0; i<9; i++) {
        *dptr++ = *mptr++ * r;
    }
    return res;
}

Matrix3x3 operator *(scalar_t r, const Matrix3x3 &mat)
{
	Matrix3x3 res;
    const scalar_t *mptr = mat.data[0];
    scalar_t *dptr = res.data[0];

    for(int i=0; i<9; i++) {
        *dptr++ = *mptr++ * r;
    }
    return res;
}

Vector3 operator *(const Matrix3x3 &mat, const Vector3 &vec)
{
	Vector3 res;

	res.x = (mat[0][0] * vec.x) + (mat[0][1] * vec.y) + (mat[0][2] * vec.z);
	res.y = (mat[1][0] * vec.x) + (mat[1][1] * vec.y) + (mat[1][2] * vec.z);
	res.z = (mat[2][0] * vec.x) + (mat[2][1] * vec.y) + (mat[2][2] * vec.z);

	return res;
}

void operator *=(Matrix3x3 &mat, scalar_t r)
{
	scalar_t *mptr = mat.data[0];

    for(int i=0; i<9; i++) {
        *mptr++ *= r;
    }
}

void Matrix3x3::translate(const Vector2 &t)
{
    Matrix3x3 tmat(1, 0, t.x, 0, 1, t.y, 0, 0, 1);
    *this *= tmat;
}

void Matrix3x3::set_translation(const Vector2 &t)
{
    *this = Matrix3x3(1, 0, t.x, 0, 1, t.y, 0, 0, 1);
}

void Matrix3x3::rotate(scalar_t angle)
{
    scalar_t cos_a = cos(angle);
    scalar_t sin_a = sin(angle);
    Matrix3x3 m(cos_a, -sin_a, 0, sin_a, cos_a, 0, 0, 0, 1);
    *this *= m;
}

void Matrix3x3::rotate(const Vector3 &euler)
{
    Matrix3x3 xrot, yrot, zrot;
    xrot = Matrix3x3(1, 0, 0, 0, cos(euler.x), -sin(euler.x), 0, sin(euler.x), cos(euler.x));
    yrot = Matrix3x3(cos(euler.y), 0, sin(euler.y), 0, 1, 0, -sin(euler.y), 0, cos(euler.y));
    zrot = Matrix3x3(cos(euler.z), -sin(euler.z), 0, sin(euler.z), cos(euler.z), 0, 0, 0, 1);
    *this *= xrot * yrot * zrot;
}

void Matrix3x3::rotate(const Vector3 &axis, scalar_t angle)
{
	scalar_t sina = (scalar_t)sin(angle);
	scalar_t cosa = (scalar_t)cos(angle);
	scalar_t invcosa = 1-cosa;
	scalar_t nxsq = axis.x * axis.x;
	scalar_t nysq = axis.y * axis.y;
	scalar_t nzsq = axis.z * axis.z;

	Matrix3x3 xform;
	xform.data[0][0] = nxsq + (1-nxsq) * cosa;
	xform.data[0][1] = axis.x * axis.y * invcosa - axis.z * sina;
	xform.data[0][2] = axis.x * axis.z * invcosa + axis.y * sina;
	xform.data[1][0] = axis.x * axis.y * invcosa + axis.z * sina;
	xform.data[1][1] = nysq + (1-nysq) * cosa;
	xform.data[1][2] = axis.y * axis.z * invcosa - axis.x * sina;
	xform.data[2][0] = axis.x * axis.z * invcosa - axis.y * sina;
	xform.data[2][1] = axis.y * axis.z * invcosa + axis.x * sina;
	xform.data[2][2] = nzsq + (1-nzsq) * cosa;

	*this *= xform;
}

void Matrix3x3::set_rotation(scalar_t angle)
{
	scalar_t cos_a = cos(angle);
	scalar_t sin_a = sin(angle);
	*this = Matrix3x3(cos_a, -sin_a, 0, sin_a, cos_a, 0, 0, 0, 1);
}

void Matrix3x3::set_rotation(const Vector3 &euler)
{
	Matrix3x3 xrot, yrot, zrot;
	xrot = Matrix3x3(1, 0, 0, 0, cos(euler.x), -sin(euler.x), 0, sin(euler.x), cos(euler.x));
	yrot = Matrix3x3(cos(euler.y), 0, sin(euler.y), 0, 1, 0, -sin(euler.y), 0, cos(euler.y));
	zrot = Matrix3x3(cos(euler.z), -sin(euler.z), 0, sin(euler.z), cos(euler.z), 0, 0, 0, 1);
	*this = xrot * yrot * zrot;
}

void Matrix3x3::set_rotation(const Vector3 &axis, scalar_t angle)
{
	scalar_t sina = (scalar_t)sin(angle);
	scalar_t cosa = (scalar_t)cos(angle);
	scalar_t invcosa = 1-cosa;
	scalar_t nxsq = axis.x * axis.x;
	scalar_t nysq = axis.y * axis.y;
	scalar_t nzsq = axis.z * axis.z;

    reset_identity();
    data[0][0] = nxsq + (1-nxsq) * cosa;
    data[0][1] = axis.x * axis.y * invcosa - axis.z * sina;
    data[0][2] = axis.x * axis.z * invcosa + axis.y * sina;
    data[1][0] = axis.x * axis.y * invcosa + axis.z * sina;
    data[1][1] = nysq + (1-nysq) * cosa;
    data[1][2] = axis.y * axis.z * invcosa - axis.x * sina;
    data[2][0] = axis.x * axis.z * invcosa - axis.y * sina;
    data[2][1] = axis.y * axis.z * invcosa + axis.x * sina;
    data[2][2] = nzsq + (1-nzsq) * cosa;
}

void Matrix3x3::scale(const Vector3 &vec)
{
    Matrix3x3 mat(vec.x, 0, 0, 0, vec.y, 0, 0, 0, vec.z);
    *this *= mat;
}

void Matrix3x3::set_scaling(const Vector3 &vec)
{
    *this = Matrix3x3(vec.x, 0, 0, 0, vec.y, 0, 0, 0, vec.z);
}

void Matrix3x3::set_column_vector(const Vector3 &vec, unsigned int index)
{
	data[0][index] = vec.x;
	data[1][index] = vec.y;
	data[2][index] = vec.z;
}

void Matrix3x3::set_row_vector(const Vector3 &vec, unsigned int index)
{
	data[index][0] = vec.x;
	data[index][1] = vec.y;
	data[index][2] = vec.z;
}

Vector3 Matrix3x3::get_column_vector(unsigned int index) const
{
	return Vector3(data[0][index], data[1][index], data[2][index]);
}

Vector3 Matrix3x3::get_row_vector(unsigned int index) const
{
	return Vector3(data[index][0], data[index][1], data[index][2]);
}

void Matrix3x3::transpose()
{
    Matrix3x3 tmp = *this;
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++) {
            data[i][j] = tmp[j][i];
        }
    }
}

Matrix3x3 Matrix3x3::transposed() const
{
    Matrix3x3 res;
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++) {
            res[i][j] = data[j][i];
        }
    }
    return res;
}

scalar_t Matrix3x3::determinant() const
{
	return	data[0][0] * (data[1][1] * data[2][2] - data[1][2] * data[2][1]) -
			data[0][1] * (data[1][0] * data[2][2] - data[1][2] * data[2][0]) +
			data[0][2] * (data[1][0] * data[2][1] - data[1][1] * data[2][0]);
}

Matrix3x3 Matrix3x3::adjoint() const
{
	Matrix3x3 coef;

	coef.data[0][0] =  (data[1][1] * data[2][2] - data[1][2] * data[2][1]);
	coef.data[0][1] =  (data[1][0] * data[2][2] - data[1][2] * data[2][0]);
	coef.data[0][2] =  (data[1][0] * data[2][1] - data[1][1] * data[2][0]);
	coef.data[1][0] =  (data[0][1] * data[2][2] - data[0][2] * data[2][1]);
	coef.data[1][1] =  (data[0][0] * data[2][2] - data[0][2] * data[2][0]);
	coef.data[1][2] =  (data[0][0] * data[2][1] - data[0][1] * data[2][0]);
	coef.data[2][0] =  (data[0][1] * data[1][2] - data[0][2] * data[1][1]);
	coef.data[2][1] =  (data[0][0] * data[1][2] - data[0][2] * data[1][0]);
	coef.data[2][2] =  (data[0][0] * data[1][1] - data[0][1] * data[1][0]);

	coef.transpose();

	for(int i=0; i<3; i++) {
		for(int j=0; j<3; j++) {
			coef.data[i][j] = j%2 ? -coef.data[i][j] : coef.data[i][j];
			if(i%2)
				coef.data[i][j] = -coef.data[i][j];
		}
	}

	return coef;
}

Matrix3x3 Matrix3x3::inverse() const
{
	Matrix3x3 adjMat = adjoint();
    return adjMat * (1.0f / determinant());
}
std::ostream &operator <<(std::ostream &out, const Matrix3x3 &mat)
{
    for(int i=0; i<3; i++)
	{
        char str[100];
        sprintf(str, "[ %12.5f, %12.5f, %12.5f ]\n", (float)mat.data[i][0], (float)mat.data[i][1], (float)mat.data[i][2]);
        out << str;
    }
    return out;
}

const Matrix4x4 Matrix4x4::identity = Matrix4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

Matrix4x4::Matrix4x4()
{
    *this = identity;
}

Matrix4x4::Matrix4x4(   scalar_t m11, scalar_t m12, scalar_t m13, scalar_t m14,
                        scalar_t m21, scalar_t m22, scalar_t m23, scalar_t m24,
                        scalar_t m31, scalar_t m32, scalar_t m33, scalar_t m34,
                        scalar_t m41, scalar_t m42, scalar_t m43, scalar_t m44)
{
    data[0][0] = m11; data[0][1] = m12; data[0][2] = m13; data[0][3] = m14;
    data[1][0] = m21; data[1][1] = m22; data[1][2] = m23; data[1][3] = m24;
    data[2][0] = m31; data[2][1] = m32; data[2][2] = m33; data[2][3] = m34;
    data[3][0] = m41; data[3][1] = m42; data[3][2] = m43; data[3][3] = m44;
}

Matrix4x4::Matrix4x4(const mat4x4_t m)
{
    memcpy(data, m, sizeof(mat4x4_t));
}

Matrix4x4::Matrix4x4(const Matrix3x3 &mat3)
{
    reset_identity();
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++) {
            data[i][j] = mat3[i][j];
        }
    }
}

Matrix4x4 operator +(const Matrix4x4 &m1, const Matrix4x4 &m2)
{
	Matrix4x4 res;
	const scalar_t *op1 = m1.data[0], *op2 = m2.data[0];
	scalar_t *dest = res.data[0];

    for(int i=0; i<16; i++) {
        *dest++ = *op1++ + *op2++;
    }
    return res;
}

Matrix4x4 operator -(const Matrix4x4 &m1, const Matrix4x4 &m2)
{
    Matrix4x4 res;
    const scalar_t *op1 = m1.data[0], *op2 = m2.data[0];
    scalar_t *dest = res.data[0];

    for(int i=0; i<16; i++) {
        *dest++ = *op1++ - *op2++;
    }
    return res;
}

Matrix4x4 operator *(const Matrix4x4 &m1, const Matrix4x4 &m2)
{
    Matrix4x4 res;

	for(int i=0; i<4; i++) {
		for(int j=0; j<4; j++) {
			res.data[i][j] =	m1.data[i][0] * m2.data[0][j] + 
									m1.data[i][1] * m2.data[1][j] + 
									m1.data[i][2] * m1.data[i][2] +
									m1.data[i][3] * m2.data[3][j];
        }
    }
    return res;
}

void operator +=(Matrix4x4 &m1, const Matrix4x4 &m2)
{
	scalar_t *op1 = m1.data[0];
	const scalar_t *op2 = m2.data[0];

	for(int i=0; i<16; i++) {
		*op1++ += *op2++;
	}
}

void operator -=(Matrix4x4 &m1, const Matrix4x4 &m2)
{
	scalar_t *op1 = m1.data[0];
	const scalar_t *op2 = m2.data[0];

	for(int i=0; i<16; i++) {
		*op1++ -= *op2++;
	}
}

void operator *=(Matrix4x4 &m1, const Matrix4x4 &m2)
{
    Matrix4x4 res;
    for(int i=0; i<4; i++) {
        for(int j=0; j<4; j++) {
			res.data[i][j] =	m1.data[i][0] * m2.data[0][j] + 
									m1.data[i][1] * m2.data[1][j] + 
									m1.data[i][2] * m1.data[i][2] +
									m1.data[i][3] * m2.data[3][j];

        }
    }
    memcpy(m1.data, res.data, 16 * sizeof(scalar_t));
}

Matrix4x4 operator *(const Matrix4x4 &mat, scalar_t r)
{
	Matrix4x4 res;
	const scalar_t *mptr = mat.data[0];
	scalar_t *dptr = res.data[0];

	for(int i=0; i<16; i++) {
		*dptr++ = *mptr++ * r;
	}
	return res;
}

Matrix4x4 operator *(scalar_t r, const Matrix4x4 &mat)
{
	Matrix4x4 res;
	const scalar_t *mptr = mat.data[0];
	scalar_t *dptr = res.data[0];

	for(int i=0; i<16; i++) {
		*dptr++ = *mptr++ * r;
	}
	return res;
}

Vector4 operator *(const Matrix4x4 &mat, const Vector4 &vec)
{
	Vector4 res;

	res.x = (mat[0][0] * vec.x) + (mat[0][1] * vec.y) + (mat[0][2] * vec.z) + (mat[0][3] * vec.w);
	res.y = (mat[1][0] * vec.x) + (mat[1][1] * vec.y) + (mat[1][2] * vec.z) + (mat[1][3] * vec.w);
	res.z = (mat[2][0] * vec.x) + (mat[2][1] * vec.y) + (mat[2][2] * vec.z) + (mat[2][3] * vec.w);
	res.w = (mat[3][0] * vec.x) + (mat[3][1] * vec.y) + (mat[3][2] * vec.z) + (mat[3][3] * vec.w);
	
	return res;
}

void operator *=(Matrix4x4 &mat, scalar_t r)
{
	scalar_t *mptr = mat.data[0];

	for(int i=0; i<16; i++) {
		*mptr++ *= r;
	}
}

void Matrix4x4::translate(const Vector3 &trans)
{
	Matrix4x4 tmat(1, 0, 0, trans.x, 0, 1, 0, trans.y, 0, 0, 1, trans.z, 0, 0, 0, 1);
	*this *= tmat;
}

void Matrix4x4::set_translation(const Vector3 &trans)
{
	*this = Matrix4x4(1, 0, 0, trans.x, 0, 1, 0, trans.y, 0, 0, 1, trans.z, 0, 0, 0, 1);
}

void Matrix4x4::rotate(const Vector3 &euler)
{
	Matrix3x3 xrot, yrot, zrot;
    xrot = Matrix3x3(1, 0, 0, 0, cos(euler.x), -sin(euler.x), 0, sin(euler.x), cos(euler.x));
	yrot = Matrix3x3(cos(euler.y), 0, sin(euler.y), 0, 1, 0, -sin(euler.y), 0, cos(euler.y));
	zrot = Matrix3x3(cos(euler.z), -sin(euler.z), 0, sin(euler.z), cos(euler.z), 0, 0, 0, 1);
	*this *= Matrix4x4(xrot * yrot * zrot);
}

void Matrix4x4::rotate(const Vector3 &axis, scalar_t angle)
{
	scalar_t sina = (scalar_t)sin(angle);
	scalar_t cosa = (scalar_t)cos(angle);
	scalar_t invcosa = 1-cosa;
	scalar_t nxsq = axis.x * axis.x;
	scalar_t nysq = axis.y * axis.y;
	scalar_t nzsq = axis.z * axis.z;

	Matrix3x3 xform;
	xform.data[0][0] = nxsq + (1-nxsq) * cosa;
	xform.data[0][1] = axis.x * axis.y * invcosa - axis.z * sina;
	xform.data[0][2] = axis.x * axis.z * invcosa + axis.y * sina;
	xform.data[1][0] = axis.x * axis.y * invcosa + axis.z * sina;
	xform.data[1][1] = nysq + (1-nysq) * cosa;
	xform.data[1][2] = axis.y * axis.z * invcosa - axis.x * sina;
	xform.data[2][0] = axis.x * axis.z * invcosa - axis.y * sina;
	xform.data[2][1] = axis.y * axis.z * invcosa + axis.x * sina;
	xform.data[2][2] = nzsq + (1-nzsq) * cosa;

	*this *= Matrix4x4(xform);
}

void Matrix4x4::set_rotation(const Vector3 &euler)
{
	Matrix3x3 xrot, yrot, zrot;
	xrot = Matrix3x3(1, 0, 0, 0, cos(euler.x), -sin(euler.x), 0, sin(euler.x), cos(euler.x));
	yrot = Matrix3x3(cos(euler.y), 0, sin(euler.y), 0, 1, 0, -sin(euler.y), 0, cos(euler.y));
	zrot = Matrix3x3(cos(euler.z), -sin(euler.z), 0, sin(euler.z), cos(euler.z), 0, 0, 0, 1);
	*this = Matrix4x4(xrot * yrot * zrot);
}

void Matrix4x4::set_rotation(const Vector3 &axis, scalar_t angle)
{
	scalar_t sina = (scalar_t)sin(angle);
	scalar_t cosa = (scalar_t)cos(angle);
	scalar_t invcosa = 1-cosa;
	scalar_t nxsq = axis.x * axis.x;
	scalar_t nysq = axis.y * axis.y;
	scalar_t nzsq = axis.z * axis.z;

	reset_identity();
	data[0][0] = nxsq + (1-nxsq) * cosa;
	data[0][1] = axis.x * axis.y * invcosa - axis.z * sina;
	data[0][2] = axis.x * axis.z * invcosa + axis.y * sina;
	data[1][0] = axis.x * axis.y * invcosa + axis.z * sina;
	data[1][1] = nysq + (1-nysq) * cosa;
	data[1][2] = axis.y * axis.z * invcosa - axis.x * sina;
	data[2][0] = axis.x * axis.z * invcosa - axis.y * sina;
	data[2][1] = axis.y * axis.z * invcosa + axis.x * sina;
	data[2][2] = nzsq + (1-nzsq) * cosa;
}

void Matrix4x4::scale(const Vector4 &vec)
{
	Matrix4x4 smat(vec.x, 0, 0, 0, 0, vec.y, 0, 0, 0, 0, vec.z, 0, 0, 0, 0, vec.w);
	*this *= smat;
}

void Matrix4x4::set_scaling(const Vector4 &vec)
{
	 *this = Matrix4x4(vec.x, 0, 0, 0, 0, vec.y, 0, 0, 0, 0, vec.z, 0, 0, 0, 0, vec.w);
}

void Matrix4x4::set_column_vector(const Vector4 &vec, unsigned int index)
{
	data[0][index] = vec.x;
	data[1][index] = vec.y;
	data[2][index] = vec.z;
	data[3][index] = vec.w;
}

void Matrix4x4::set_row_vector(const Vector4 &vec, unsigned int index)
{
	data[index][0] = vec.x;
	data[index][1] = vec.y;
	data[index][2] = vec.z;
	data[index][3] = vec.w;
}

Vector4 Matrix4x4::get_column_vector(unsigned int index) const
{
	return Vector4(data[0][index], data[1][index], data[2][index], data[3][index]);
}

Vector4 Matrix4x4::get_row_vector(unsigned int index) const
{
	return Vector4(data[index][0], data[index][1], data[index][2], data[index][3]);
}

void Matrix4x4::transpose()
{
	Matrix4x4 tmp = *this;
	for(int i=0; i<4; i++) {
		for(int j=0; j<4; j++) {
			data[i][j] = tmp[j][i];
		}
	}
}

Matrix4x4 Matrix4x4::transposed() const
{
    Matrix4x4 res;
    for(int i=0; i<4; i++) {
        for(int j=0; j<4; j++) {
            res[i][j] = data[j][i];
        }
    }
    return res;
}

scalar_t Matrix4x4::determinant() const
{
	scalar_t det11 =	(data[1][1] * (data[2][2] * data[3][3] - data[3][2] * data[2][3])) -
					(data[1][2] * (data[2][1] * data[3][3] - data[3][1] * data[2][3])) +
					(data[1][3] * (data[2][1] * data[3][2] - data[3][1] * data[2][2]));

	scalar_t det12 =	(data[1][0] * (data[2][2] * data[3][3] - data[3][2] * data[2][3])) -
					(data[1][2] * (data[2][0] * data[3][3] - data[3][0] * data[2][3])) +
					(data[1][3] * (data[2][0] * data[3][2] - data[3][0] * data[2][2]));

	scalar_t det13 =	(data[1][0] * (data[2][1] * data[3][3] - data[3][1] * data[2][3])) -
					(data[1][1] * (data[2][0] * data[3][3] - data[3][0] * data[2][3])) +
					(data[1][3] * (data[2][0] * data[3][1] - data[3][0] * data[2][1]));

	scalar_t det14 =	(data[1][0] * (data[2][1] * data[3][2] - data[3][1] * data[2][2])) -
					(data[1][1] * (data[2][0] * data[3][2] - data[3][0] * data[2][2])) +
					(data[1][2] * (data[2][0] * data[3][1] - data[3][0] * data[2][1]));

	return data[0][0] * det11 - data[0][1] * det12 + data[0][2] * det13 - data[0][3] * det14;
}

Matrix4x4 Matrix4x4::adjoint() const
{
	Matrix4x4 coef;

	coef.data[0][0] =	(data[1][1] * (data[2][2] * data[3][3] - data[3][2] * data[2][3])) -
							(data[1][2] * (data[2][1] * data[3][3] - data[3][1] * data[2][3])) +
							(data[1][3] * (data[2][1] * data[3][2] - data[3][1] * data[2][2]));

	coef.data[0][1] =  	(data[1][0] * (data[2][2] * data[3][3] - data[3][2] * data[2][3])) -
							(data[1][2] * (data[2][0] * data[3][3] - data[3][0] * data[2][3])) +
							(data[1][3] * (data[2][0] * data[3][2] - data[3][0] * data[2][2]));

	coef.data[0][2] =  	(data[1][0] * (data[2][1] * data[3][3] - data[3][1] * data[2][3])) -
							(data[1][1] * (data[2][0] * data[3][3] - data[3][0] * data[2][3])) +
							(data[1][3] * (data[2][0] * data[3][1] - data[3][0] * data[2][1]));

	coef.data[0][3] =  	(data[1][0] * (data[2][1] * data[3][2] - data[3][1] * data[2][2])) -
							(data[1][1] * (data[2][0] * data[3][2] - data[3][0] * data[2][2])) +
							(data[1][2] * (data[2][0] * data[3][1] - data[3][0] * data[2][1]));

	coef.data[1][0] =  	(data[0][1] * (data[2][2] * data[3][3] - data[3][2] * data[2][3])) -
							(data[0][2] * (data[2][1] * data[3][3] - data[3][1] * data[2][3])) +
							(data[0][3] * (data[2][1] * data[3][2] - data[3][1] * data[2][2]));

	coef.data[1][1] =  	(data[0][0] * (data[2][2] * data[3][3] - data[3][2] * data[2][3])) -
							(data[0][2] * (data[2][0] * data[3][3] - data[3][0] * data[2][3])) +
							(data[0][3] * (data[2][0] * data[3][2] - data[3][0] * data[2][2]));

	coef.data[1][2] =  	(data[0][0] * (data[2][1] * data[3][3] - data[3][1] * data[2][3])) -
							(data[0][1] * (data[2][0] * data[3][3] - data[3][0] * data[2][3])) +
							(data[0][3] * (data[2][0] * data[3][1] - data[3][0] * data[2][1]));

	coef.data[1][3] =  	(data[0][0] * (data[2][1] * data[3][2] - data[3][1] * data[2][2])) -
							(data[0][1] * (data[2][0] * data[3][2] - data[3][0] * data[2][2])) +
							(data[0][2] * (data[2][0] * data[3][1] - data[3][0] * data[2][1]));

	coef.data[2][0] =  	(data[0][1] * (data[1][2] * data[3][3] - data[3][2] * data[1][3])) -
							(data[0][2] * (data[1][1] * data[3][3] - data[3][1] * data[1][3])) +
							(data[0][3] * (data[1][1] * data[3][2] - data[3][1] * data[1][2]));

	coef.data[2][1] =  	(data[0][0] * (data[1][2] * data[3][3] - data[3][2] * data[1][3])) -
							(data[0][2] * (data[1][0] * data[3][3] - data[3][0] * data[1][3])) +
							(data[0][3] * (data[1][0] * data[3][2] - data[3][0] * data[1][2]));

	coef.data[2][2] =  	(data[0][0] * (data[1][1] * data[3][3] - data[3][1] * data[1][3])) -
							(data[0][1] * (data[1][0] * data[3][3] - data[3][0] * data[1][3])) +
							(data[0][3] * (data[1][0] * data[3][1] - data[3][0] * data[1][1]));

	coef.data[2][3] =  	(data[0][0] * (data[1][1] * data[3][2] - data[3][1] * data[1][2])) -
							(data[0][1] * (data[1][0] * data[3][2] - data[3][0] * data[1][2])) +
							(data[0][2] * (data[1][0] * data[3][1] - data[3][0] * data[1][1]));

	coef.data[3][0] =  	(data[0][1] * (data[1][2] * data[2][3] - data[2][2] * data[1][3])) -
							(data[0][2] * (data[1][1] * data[2][3] - data[2][1] * data[1][3])) +
							(data[0][3] * (data[1][1] * data[2][2] - data[2][1] * data[1][2]));

	coef.data[3][1] =  	(data[0][0] * (data[1][2] * data[2][3] - data[2][2] * data[1][3])) -
							(data[0][2] * (data[1][0] * data[2][3] - data[2][0] * data[1][3])) +
							(data[0][3] * (data[1][0] * data[2][2] - data[2][0] * data[1][2]));

	coef.data[3][2] =  	(data[0][0] * (data[1][1] * data[2][3] - data[2][1] * data[1][3])) -
							(data[0][1] * (data[1][0] * data[2][3] - data[2][0] * data[1][3])) +
							(data[0][3] * (data[1][0] * data[2][1] - data[2][0] * data[1][1]));

	coef.data[3][3] =  	(data[0][0] * (data[1][1] * data[2][2] - data[2][1] * data[1][2])) -
							(data[0][1] * (data[1][0] * data[2][2] - data[2][0] * data[1][2])) +
							(data[0][2] * (data[1][0] * data[2][1] - data[2][0] * data[1][1]));

	coef.transpose();

	for(int i=0; i<4; i++) {
		for(int j=0; j<4; j++) {
			coef.data[i][j] = j%2 ? -coef.data[i][j] : coef.data[i][j];
			if(i%2)
				coef.data[i][j] = -coef.data[i][j];
		}
	}

	return coef;
}

Matrix4x4 Matrix4x4::inverse() const
{
	Matrix4x4 adjMat = adjoint();
    return adjMat * (1.0f / determinant());
}

std::ostream &operator <<(std::ostream &out, const Matrix4x4 &mat)
{
    for(int i=0; i<4; i++)
	{
        char str[100];
        sprintf(str, "[ %12.5f, %12.5f, %12.5f, %12.5f ]\n", (float)mat.data[i][0], (float)mat.data[i][1], (float)mat.data[i][2], (float)mat.data[i][3]);
        out << str;
    }
    return out;
}

#endif
