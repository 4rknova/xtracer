#include <cmath>
#include <cstdio>
#include "matrix.h"
#include "vector.h"

namespace nmath {

const Matrix3x3f Matrix3x3f::identity = Matrix3x3f(1, 0, 0, 0, 1, 0, 0, 0, 1);

Matrix3x3f::Matrix3x3f()
{
    *this = identity;
}

Matrix3x3f::Matrix3x3f(	scalar_t m11, scalar_t m12, scalar_t m13,
						scalar_t m21, scalar_t m22, scalar_t m23,
						scalar_t m31, scalar_t m32, scalar_t m33)
{
	data[0][0] = m11; data[0][1] = m12; data[0][2] = m13;
	data[1][0] = m21; data[1][1] = m22; data[1][2] = m23;
	data[2][0] = m31; data[2][1] = m32; data[2][2] = m33;
}

Matrix3x3f::Matrix3x3f(const Matrix4x4f &mat4)
{
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++) {
            data[i][j] = mat4[i][j];
        }
    }
}

Matrix3x3f operator +(const Matrix3x3f &m1, const Matrix3x3f &m2)
{
	Matrix3x3f res;
	const scalar_t *op1 = m1.data[0], *op2 = m2.data[0];
    scalar_t *dest = res.data[0];

    for(int i=0; i<9; i++) {
        *dest++ = *op1++ + *op2++;
    }
    return res;
}

Matrix3x3f operator -(const Matrix3x3f &m1, const Matrix3x3f &m2)
{
    Matrix3x3f res;
    const scalar_t *op1 = m1.data[0], *op2 = m2.data[0];
    scalar_t *dest = res.data[0];

    for(int i=0; i<9; i++) {
        *dest++ = *op1++ - *op2++;
    }
    return res;
}

Matrix3x3f operator *(const Matrix3x3f &m1, const Matrix3x3f &m2)
{
	Matrix3x3f res;
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++) {
            res.data[i][j] = m1.data[i][0] * m2.data[0][j] + m1.data[i][1] * m2.data[1][j] + m1.data[i][2] * m2.data[2][j];
        }
    }
    return res;
}

void operator +=(Matrix3x3f &m1, const Matrix3x3f &m2)
{
    scalar_t *op1 = m1.data[0];
    const scalar_t *op2 = m2.data[0];

    for(int i=0; i<9; i++) {
        *op1++ += *op2++;
    }
}

void operator -=(Matrix3x3f &m1, const Matrix3x3f &m2)
{
    scalar_t *op1 = m1.data[0];
    const scalar_t *op2 = m2.data[0];

    for(int i=0; i<9; i++) {
        *op1++ -= *op2++;
    }
}

void operator *=(Matrix3x3f &m1, const Matrix3x3f &m2)
{
    Matrix3x3f res;
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++) {
            res.data[i][j] = m1.data[i][0] * m2.data[0][j] + m1.data[i][1] * m2.data[1][j] + m1.data[i][2] * m2.data[2][j];
        }
    }
    memcpy(m1.data, res.data, 9 * sizeof(scalar_t));
}

Matrix3x3f operator *(const Matrix3x3f &mat, scalar_t r)
{
	Matrix3x3f res;
    const scalar_t *mptr = mat.data[0];
    scalar_t *dptr = res.data[0];

    for(int i=0; i<9; i++) {
        *dptr++ = *mptr++ * r;
    }
    return res;
}

Matrix3x3f operator *(scalar_t r, const Matrix3x3f &mat)
{
	Matrix3x3f res;
    const scalar_t *mptr = mat.data[0];
    scalar_t *dptr = res.data[0];

    for(int i=0; i<9; i++) {
        *dptr++ = *mptr++ * r;
    }
    return res;
}

Vector3f operator *(const Matrix3x3f &mat, const Vector3f &vec)
{
	Vector3f res;

	res.x = (mat[0][0] * vec.x) + (mat[0][1] * vec.y) + (mat[0][2] * vec.z);
	res.y = (mat[1][0] * vec.x) + (mat[1][1] * vec.y) + (mat[1][2] * vec.z);
	res.z = (mat[2][0] * vec.x) + (mat[2][1] * vec.y) + (mat[2][2] * vec.z);

	return res;
}

void operator *=(Matrix3x3f &mat, scalar_t r)
{
	scalar_t *mptr = mat.data[0];

    for(int i=0; i<9; i++) {
        *mptr++ *= r;
    }
}

void Matrix3x3f::translate(const Vector2f &t)
{
    Matrix3x3f tmat(1, 0, t.x, 0, 1, t.y, 0, 0, 1);
    *this *= tmat;
}

void Matrix3x3f::set_translation(const Vector2f &t)
{
    *this = Matrix3x3f(1, 0, t.x, 0, 1, t.y, 0, 0, 1);
}

void Matrix3x3f::rotate(scalar_t angle)
{
    scalar_t cos_a = cos(angle);
    scalar_t sin_a = sin(angle);
    Matrix3x3f m(cos_a, -sin_a, 0, sin_a, cos_a, 0, 0, 0, 1);
    *this *= m;
}

void Matrix3x3f::rotate(const Vector3f &euler)
{
    Matrix3x3f xrot, yrot, zrot;
    xrot = Matrix3x3f(1, 0, 0, 0, cos(euler.x), -sin(euler.x), 0, sin(euler.x), cos(euler.x));
    yrot = Matrix3x3f(cos(euler.y), 0, sin(euler.y), 0, 1, 0, -sin(euler.y), 0, cos(euler.y));
    zrot = Matrix3x3f(cos(euler.z), -sin(euler.z), 0, sin(euler.z), cos(euler.z), 0, 0, 0, 1);
    *this *= xrot * yrot * zrot;
}

void Matrix3x3f::rotate(const Vector3f &axis, scalar_t angle)
{
	scalar_t sina = (scalar_t)sin(angle);
	scalar_t cosa = (scalar_t)cos(angle);
	scalar_t invcosa = 1-cosa;
	scalar_t nxsq = axis.x * axis.x;
	scalar_t nysq = axis.y * axis.y;
	scalar_t nzsq = axis.z * axis.z;

	Matrix3x3f xform;
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

void Matrix3x3f::set_rotation(scalar_t angle)
{
	scalar_t cos_a = cos(angle);
	scalar_t sin_a = sin(angle);
	*this = Matrix3x3f(cos_a, -sin_a, 0, sin_a, cos_a, 0, 0, 0, 1);
}

void Matrix3x3f::set_rotation(const Vector3f &euler)
{
	Matrix3x3f xrot, yrot, zrot;
	xrot = Matrix3x3f(1, 0, 0, 0, cos(euler.x), -sin(euler.x), 0, sin(euler.x), cos(euler.x));
	yrot = Matrix3x3f(cos(euler.y), 0, sin(euler.y), 0, 1, 0, -sin(euler.y), 0, cos(euler.y));
	zrot = Matrix3x3f(cos(euler.z), -sin(euler.z), 0, sin(euler.z), cos(euler.z), 0, 0, 0, 1);
	*this = xrot * yrot * zrot;
}

void Matrix3x3f::set_rotation(const Vector3f &axis, scalar_t angle)
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

void Matrix3x3f::scale(const Vector3f &vec)
{
    Matrix3x3f mat(vec.x, 0, 0, 0, vec.y, 0, 0, 0, vec.z);
    *this *= mat;
}

void Matrix3x3f::set_scaling(const Vector3f &vec)
{
    *this = Matrix3x3f(vec.x, 0, 0, 0, vec.y, 0, 0, 0, vec.z);
}

void Matrix3x3f::set_column_vector(const Vector3f &vec, unsigned int index)
{
	data[0][index] = vec.x;
	data[1][index] = vec.y;
	data[2][index] = vec.z;
}

void Matrix3x3f::set_row_vector(const Vector3f &vec, unsigned int index)
{
	data[index][0] = vec.x;
	data[index][1] = vec.y;
	data[index][2] = vec.z;
}

Vector3f Matrix3x3f::get_column_vector(unsigned int index) const
{
	return Vector3f(data[0][index], data[1][index], data[2][index]);
}

Vector3f Matrix3x3f::get_row_vector(unsigned int index) const
{
	return Vector3f(data[index][0], data[index][1], data[index][2]);
}

void Matrix3x3f::transpose()
{
    Matrix3x3f tmp = *this;
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++) {
            data[i][j] = tmp[j][i];
        }
    }
}

Matrix3x3f Matrix3x3f::transposed() const
{
    Matrix3x3f res;
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++) {
            res[i][j] = data[j][i];
        }
    }
    return res;
}

scalar_t Matrix3x3f::determinant() const
{
	return	data[0][0] * (data[1][1] * data[2][2] - data[1][2] * data[2][1]) -
			data[0][1] * (data[1][0] * data[2][2] - data[1][2] * data[2][0]) +
			data[0][2] * (data[1][0] * data[2][1] - data[1][1] * data[2][0]);
}

Matrix3x3f Matrix3x3f::adjoint() const
{
	Matrix3x3f coef;

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

Matrix3x3f Matrix3x3f::inverse() const
{
	Matrix3x3f adjMat = adjoint();
    return adjMat * (1.0f / determinant());
}

const Matrix4x4f Matrix4x4f::identity = Matrix4x4f(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

Matrix4x4f::Matrix4x4f()
{
    *this = identity;
}

Matrix4x4f::Matrix4x4f( scalar_t m11, scalar_t m12, scalar_t m13, scalar_t m14,
                        scalar_t m21, scalar_t m22, scalar_t m23, scalar_t m24,
                        scalar_t m31, scalar_t m32, scalar_t m33, scalar_t m34,
                        scalar_t m41, scalar_t m42, scalar_t m43, scalar_t m44)
{
    data[0][0] = m11; data[0][1] = m12; data[0][2] = m13; data[0][3] = m14;
    data[1][0] = m21; data[1][1] = m22; data[1][2] = m23; data[1][3] = m24;
    data[2][0] = m31; data[2][1] = m32; data[2][2] = m33; data[2][3] = m34;
    data[3][0] = m41; data[3][1] = m42; data[3][2] = m43; data[3][3] = m44;
}

Matrix4x4f::Matrix4x4f(const Matrix3x3f &mat3)
{
    reset_identity();
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++) {
            data[i][j] = mat3[i][j];
        }
    }
}

Matrix4x4f operator +(const Matrix4x4f &m1, const Matrix4x4f &m2)
{
	Matrix4x4f res;
	const scalar_t *op1 = m1.data[0], *op2 = m2.data[0];
	scalar_t *dest = res.data[0];

    for(int i=0; i<16; i++) {
        *dest++ = *op1++ + *op2++;
    }
    return res;
}

Matrix4x4f operator -(const Matrix4x4f &m1, const Matrix4x4f &m2)
{
    Matrix4x4f res;
    const scalar_t *op1 = m1.data[0], *op2 = m2.data[0];
    scalar_t *dest = res.data[0];

    for(int i=0; i<16; i++) {
        *dest++ = *op1++ - *op2++;
    }
    return res;
}

Matrix4x4f operator *(const Matrix4x4f &m1, const Matrix4x4f &m2)
{
    Matrix4x4f res;

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

void operator +=(Matrix4x4f &m1, const Matrix4x4f &m2)
{
	scalar_t *op1 = m1.data[0];
	const scalar_t *op2 = m2.data[0];

	for(int i=0; i<16; i++) {
		*op1++ += *op2++;
	}
}

void operator -=(Matrix4x4f &m1, const Matrix4x4f &m2)
{
	scalar_t *op1 = m1.data[0];
	const scalar_t *op2 = m2.data[0];

	for(int i=0; i<16; i++) {
		*op1++ -= *op2++;
	}
}

void operator *=(Matrix4x4f &m1, const Matrix4x4f &m2)
{
    Matrix4x4f res;
    for(int i=0; i<4; i++) {
        for(int j=0; j<4; j++) {
			res.data[i][j] = m1.data[i][0] * m2.data[0][j] +
							 m1.data[i][1] * m2.data[1][j] +
							 m1.data[i][2] * m2.data[2][j] +
							 m1.data[i][3] * m2.data[3][j];

        }
    }
    memcpy(m1.data, res.data, 16 * sizeof(scalar_t));
}

Matrix4x4f operator *(const Matrix4x4f &mat, scalar_t r)
{
	Matrix4x4f res;
	const scalar_t *mptr = mat.data[0];
	scalar_t *dptr = res.data[0];

	for(int i=0; i<16; i++) {
		*dptr++ = *mptr++ * r;
	}
	return res;
}

Matrix4x4f operator *(scalar_t r, const Matrix4x4f &mat)
{
	Matrix4x4f res;
	const scalar_t *mptr = mat.data[0];
	scalar_t *dptr = res.data[0];

	for(int i=0; i<16; i++) {
		*dptr++ = *mptr++ * r;
	}
	return res;
}

Vector4f operator *(const Matrix4x4f &mat, const Vector4f &vec)
{
	Vector4f res;

	res.x = (mat[0][0] * vec.x) + (mat[0][1] * vec.y) + (mat[0][2] * vec.z) + (mat[0][3] * vec.w);
	res.y = (mat[1][0] * vec.x) + (mat[1][1] * vec.y) + (mat[1][2] * vec.z) + (mat[1][3] * vec.w);
	res.z = (mat[2][0] * vec.x) + (mat[2][1] * vec.y) + (mat[2][2] * vec.z) + (mat[2][3] * vec.w);
	res.w = (mat[3][0] * vec.x) + (mat[3][1] * vec.y) + (mat[3][2] * vec.z) + (mat[3][3] * vec.w);

	return res;
}

void operator *=(Matrix4x4f &mat, scalar_t r)
{
	scalar_t *mptr = mat.data[0];

	for(int i=0; i<16; i++) {
		*mptr++ *= r;
	}
}

void Matrix4x4f::translate(const Vector3f &trans)
{
	Matrix4x4f tmat(1, 0, 0, trans.x, 0, 1, 0, trans.y, 0, 0, 1, trans.z, 0, 0, 0, 1);
	*this *= tmat;
}

void Matrix4x4f::set_translation(const Vector3f &trans)
{
	*this = Matrix4x4f(1, 0, 0, trans.x, 0, 1, 0, trans.y, 0, 0, 1, trans.z, 0, 0, 0, 1);
}

void Matrix4x4f::rotate(const Vector3f &euler)
{
	Matrix3x3f xrot, yrot, zrot;
    xrot = Matrix3x3f(1, 0, 0, 0, cos(euler.x), -sin(euler.x), 0, sin(euler.x), cos(euler.x));
	yrot = Matrix3x3f(cos(euler.y), 0, sin(euler.y), 0, 1, 0, -sin(euler.y), 0, cos(euler.y));
	zrot = Matrix3x3f(cos(euler.z), -sin(euler.z), 0, sin(euler.z), cos(euler.z), 0, 0, 0, 1);
	*this *= Matrix4x4f(xrot * yrot * zrot);
}

void Matrix4x4f::rotate(const Vector3f &axis, scalar_t angle)
{
	scalar_t sina = (scalar_t)sin(angle);
	scalar_t cosa = (scalar_t)cos(angle);
	scalar_t invcosa = 1-cosa;
	scalar_t nxsq = axis.x * axis.x;
	scalar_t nysq = axis.y * axis.y;
	scalar_t nzsq = axis.z * axis.z;

	Matrix3x3f xform;
	xform.data[0][0] = nxsq + (1-nxsq) * cosa;
	xform.data[0][1] = axis.x * axis.y * invcosa - axis.z * sina;
	xform.data[0][2] = axis.x * axis.z * invcosa + axis.y * sina;
	xform.data[1][0] = axis.x * axis.y * invcosa + axis.z * sina;
	xform.data[1][1] = nysq + (1-nysq) * cosa;
	xform.data[1][2] = axis.y * axis.z * invcosa - axis.x * sina;
	xform.data[2][0] = axis.x * axis.z * invcosa - axis.y * sina;
	xform.data[2][1] = axis.y * axis.z * invcosa + axis.x * sina;
	xform.data[2][2] = nzsq + (1-nzsq) * cosa;

	*this *= Matrix4x4f(xform);
}

void Matrix4x4f::set_rotation(const Vector3f &euler)
{
	Matrix3x3f xrot, yrot, zrot;
	xrot = Matrix3x3f(1, 0, 0, 0, cos(euler.x), -sin(euler.x), 0, sin(euler.x), cos(euler.x));
	yrot = Matrix3x3f(cos(euler.y), 0, sin(euler.y), 0, 1, 0, -sin(euler.y), 0, cos(euler.y));
	zrot = Matrix3x3f(cos(euler.z), -sin(euler.z), 0, sin(euler.z), cos(euler.z), 0, 0, 0, 1);
	*this = Matrix4x4f(xrot * yrot * zrot);
}

void Matrix4x4f::set_rotation(const Vector3f &axis, scalar_t angle)
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

void Matrix4x4f::scale(const Vector4f &vec)
{
	Matrix4x4f smat(vec.x, 0, 0, 0, 0, vec.y, 0, 0, 0, 0, vec.z, 0, 0, 0, 0, vec.w);
	*this *= smat;
}

void Matrix4x4f::set_scaling(const Vector4f &vec)
{
	 *this = Matrix4x4f(vec.x, 0, 0, 0, 0, vec.y, 0, 0, 0, 0, vec.z, 0, 0, 0, 0, vec.w);
}

void Matrix4x4f::set_column_vector(const Vector4f &vec, unsigned int index)
{
	data[0][index] = vec.x;
	data[1][index] = vec.y;
	data[2][index] = vec.z;
	data[3][index] = vec.w;
}

void Matrix4x4f::set_row_vector(const Vector4f &vec, unsigned int index)
{
	data[index][0] = vec.x;
	data[index][1] = vec.y;
	data[index][2] = vec.z;
	data[index][3] = vec.w;
}

Vector4f Matrix4x4f::get_column_vector(unsigned int index) const
{
	return Vector4f(data[0][index], data[1][index], data[2][index], data[3][index]);
}

Vector4f Matrix4x4f::get_row_vector(unsigned int index) const
{
	return Vector4f(data[index][0], data[index][1], data[index][2], data[index][3]);
}

void Matrix4x4f::transpose()
{
	Matrix4x4f tmp = *this;
	for(int i=0; i<4; i++) {
		for(int j=0; j<4; j++) {
			data[i][j] = tmp[j][i];
		}
	}
}

Matrix4x4f Matrix4x4f::transposed() const
{
    Matrix4x4f res;
    for(int i=0; i<4; i++) {
        for(int j=0; j<4; j++) {
            res[i][j] = data[j][i];
        }
    }
    return res;
}

scalar_t Matrix4x4f::determinant() const
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

Matrix4x4f Matrix4x4f::adjoint() const
{
	Matrix4x4f coef;

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

Matrix4x4f Matrix4x4f::inverse() const
{
	Matrix4x4f adjMat = adjoint();
    return adjMat * (1.0f / determinant());
}

} /* namespace nmath */
