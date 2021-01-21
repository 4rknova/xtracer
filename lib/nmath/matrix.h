#ifndef NMATH_MATRIX_H_INCLUDED
#define NMATH_MATRIX_H_INCLUDED

#include "precision.h"
#include "types.h"

namespace NMath {

class Matrix3x3f
{
	friend class Matrix4x4f;
	public:
		/* Constructors */
		Matrix3x3f();
		Matrix3x3f(	scalar_t m11, scalar_t m12, scalar_t m13,
					scalar_t m21, scalar_t m22, scalar_t m23,
					scalar_t m31, scalar_t m32, scalar_t m33);
		Matrix3x3f(const Matrix4x4f &mat4);

		/* Binary operators */
		friend Matrix3x3f operator +(const Matrix3x3f &m1, const Matrix3x3f &m2);
		friend Matrix3x3f operator -(const Matrix3x3f &m1, const Matrix3x3f &m2);
		friend Matrix3x3f operator *(const Matrix3x3f &m1, const Matrix3x3f &m2);

		/* Compound binary operators */
		friend void operator +=(Matrix3x3f &m1, const Matrix3x3f &m2);
		friend void operator -=(Matrix3x3f &m1, const Matrix3x3f &m2);
		friend void operator *=(Matrix3x3f &m1, const Matrix3x3f &m2);

		/* Scalar operators */
		friend Matrix3x3f operator *(const Matrix3x3f &mat, scalar_t r);
		friend Matrix3x3f operator *(scalar_t r, const Matrix3x3f &mat);

		/* Vector operators */
		friend Vector3f operator *(const Matrix3x3f &mat, const Vector3f &vec);

		/* Compound scalar operators */
		friend void operator *=(Matrix3x3f &mat, scalar_t r);

		/* Index operator */
		inline scalar_t *operator [](int index);
		inline const scalar_t *operator [](int index) const;

		/* Reset matrix */
		inline void reset_identity();

		/* Transformations */
		void translate(const Vector2f &trans);
		void set_translation(const Vector2f &trans);

		void rotate(scalar_t angle);						/* 2d rotation */
    	void rotate(const Vector3f &euler);				/* 3d rotation with euler angles */
    	void rotate(const Vector3f &axis, scalar_t angle);	/* 3d axis/angle rotation */

		void set_rotation(scalar_t angle);
    	void set_rotation(const Vector3f &euler);
		void set_rotation(const Vector3f &axis, scalar_t angle);

		void scale(const Vector3f &vec);
		void set_scaling(const Vector3f &vec);

		/* Tuple operations */
		void set_column_vector(const Vector3f &vec, unsigned int index);
		void set_row_vector(const Vector3f &vec, unsigned int index);
		Vector3f get_column_vector(unsigned int index) const;
		Vector3f get_row_vector(unsigned int index) const;

		void transpose();
		Matrix3x3f transposed() const;
		scalar_t determinant() const;
		Matrix3x3f adjoint() const;
		Matrix3x3f inverse() const;

		static const Matrix3x3f identity;

		scalar_t data[3][3];
};

class Matrix4x4f
{
	friend class Matrix3x3f;
	public:
		/* Constructors */
		Matrix4x4f();
		Matrix4x4f(  scalar_t m11, scalar_t m12, scalar_t m13, scalar_t m14,
					scalar_t m21, scalar_t m22, scalar_t m23, scalar_t m24,
					scalar_t m31, scalar_t m32, scalar_t m33, scalar_t m34,
					scalar_t m41, scalar_t m42, scalar_t m43, scalar_t m44);
		Matrix4x4f(const Matrix3x3f &mat3);

		/* Binary operators */
		friend Matrix4x4f operator +(const Matrix4x4f &m1, const Matrix4x4f &m2);
		friend Matrix4x4f operator -(const Matrix4x4f &m1, const Matrix4x4f &m2);
		friend Matrix4x4f operator *(const Matrix4x4f &m1, const Matrix4x4f &m2);

		/* Compound binary operators */
		friend void operator +=(Matrix4x4f &m1, const Matrix4x4f &m2);
		friend void operator -=(Matrix4x4f &m1, const Matrix4x4f &m2);
		friend void operator *=(Matrix4x4f &m1, const Matrix4x4f &m2);

		/* Scalar operators */
		friend Matrix4x4f operator *(const Matrix4x4f &mat, scalar_t r);
		friend Matrix4x4f operator *(scalar_t r, const Matrix4x4f &mat);

		/* Vector operators */
		friend Vector4f operator *(const Matrix4x4f &mat, const Vector4f &vec);

		/* Compound scalar operators */
		friend void operator *=(Matrix4x4f &mat, scalar_t r);

		/* Index operator */
		inline scalar_t *operator [](int index);
		inline const scalar_t *operator [](int index) const;

		/* Reset matrix */
		inline void reset_identity();

		/* Transformations */
		void translate(const Vector3f &trans);
		void set_translation(const Vector3f &trans);

		void rotate(const Vector3f &euler);           	/* 3d rotation with euler angles */
		void rotate(const Vector3f &axis, scalar_t angle); /* 3d axis/angle rotation */
		void set_rotation(const Vector3f &euler);
		void set_rotation(const Vector3f &axis, scalar_t angle);

		void scale(const Vector4f &vec);
		void set_scaling(const Vector4f &vec);

		/* Tuple operations */
 		void set_column_vector(const Vector4f &vec, unsigned int index);
		void set_row_vector(const Vector4f &vec, unsigned int index);
		Vector4f get_column_vector(unsigned int index) const;
		Vector4f get_row_vector(unsigned int index) const;

		void transpose();
		Matrix4x4f transposed() const;
		scalar_t determinant() const;
		Matrix4x4f adjoint() const;
		Matrix4x4f inverse() const;

		static const Matrix4x4f identity;

		scalar_t data[4][4];
};

} /* namespace NMath */

#include "matrix.inl"

#endif /* NMATH_MATRIX_H_INCLUDED */
