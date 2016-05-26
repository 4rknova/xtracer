/*

	This file is part of libnmath.

	sample.inl
	Sampling functions

	Copyright (C) 2012
	Papadopoulos Nikolaos

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 3 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General
	Public License along with this program; if not, write to the
	Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
	Boston, MA 02110-1301 USA

*/

#ifndef NMATH_SAMPLE_INL_INCLUDED
#define NMATH_SAMPLE_INL_INCLUDED

#ifndef NMATH_SAMPLE_H_INCLUDED
    #error "sample.h must be included before sample.inl"
#endif /* NMATH_SAMPLE_H_INCLUDED */

#ifdef __cplusplus
	#include <cstdlib>
#endif /* __cplusplus */

#include "precision.h"
#include "prng.h"
#include "matrix.h"

namespace NMath {
	namespace Sample {

#ifdef __cplusplus

inline Vector3f sphere()
{
	scalar_t u = prng_c(0.0f, 1.0f);
	scalar_t v = prng_c(0.0f, 1.0f);

	scalar_t theta = 2.0 * PI * u;
	scalar_t phi = nmath_acos(2.0f * v - 1.0f);

	return Vector3f(nmath_cos(theta) * nmath_sin(phi),
					nmath_cos(phi),
					nmath_sin(theta) * nmath_sin(phi));
}

inline Vector3f hemisphere(const Vector3f &normal, const Vector3f &direction)
{
	scalar_t u = prng_c(0.0f, 1.0f);
	scalar_t v = prng_c(0.0f, 1.0f);

	float phi   = nmath_acos(nmath_sqrt(u));
	float theta = 2.0 * M_PI * v;

	Vector3f d;
	d.x = nmath_cos(theta) * nmath_sin(phi);
	d.y = nmath_cos(phi);
	d.z = nmath_sin(theta) * nmath_sin(phi);

	NMath::Matrix3x3f mat;

	Vector3f norm = normal.normalized();

	Vector3f xvec = cross(norm + Vector3f(EPSILON, EPSILON, EPSILON), norm).normalized();
	Vector3f yvec = norm;
	Vector3f zvec = cross(xvec, yvec).normalized();

	mat.set_column_vector(xvec, 0);
	mat.set_column_vector(yvec, 1);
	mat.set_column_vector(zvec, 2);

	d.transform(mat);

	Vector3f dirc = direction.normalized();
	float ndotl = dot(norm, dirc);
	
	return (d * (ndotl > 0 ? ndotl : 0)).normalized();
}

inline Vector3f lobe(const Vector3f &normal, const Vector3f &direction, const scalar_t exponent)
{
	 Matrix3x3f mat;
	 Vector3f refl = direction.reflected(normal);
	 
	 if (1.0f - dot(direction, normal) > EPSILON) {
		Vector3f ivec = cross(direction, refl).normalized();
		Vector3f kvec = cross(refl, ivec);
		mat.set_column_vector(ivec, 0);
		mat.set_column_vector(refl, 1);
		mat.set_column_vector(kvec, 2);
	}
	
	scalar_t u = prng_c(0.0f, 1.0f);
	scalar_t v = prng_c(0.0f, 1.0f);
	scalar_t theta = 2.0 * M_PI * u;
	scalar_t phi = nmath_acos(nmath_pow(v, 1.0 / (exponent + 1)));

	Vector3f vc(nmath_cos(theta) * nmath_sin(phi),
			    nmath_cos(phi),
			    nmath_sin(theta) * nmath_sin(phi));
	
	vc.transform(mat);

	scalar_t vdotr = dot(refl, vc);

	if (vdotr < 0.0) {
		vdotr = 0.0;
	}
	
	return (vc * nmath_pow(vdotr, exponent)).normalized();
}

#endif /* __cplusplus */

	} /* namespace Sample */
} /* namespace NMath */

#endif /* NMATH_SAMPLE_INL_INCLUDED */
