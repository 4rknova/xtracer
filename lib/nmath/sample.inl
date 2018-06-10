#ifndef NMATH_SAMPLE_INL_INCLUDED
#define NMATH_SAMPLE_INL_INCLUDED

#ifndef NMATH_SAMPLE_H_INCLUDED
    #error "sample.h must be included before sample.inl"
#endif /* NMATH_SAMPLE_H_INCLUDED */

#include "precision.h"
#include "prng.h"
#include "matrix.h"

namespace NMath {
	namespace Sample {

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

	if (vdotr < 0.0) vdotr = 0.0;

	return (vc * nmath_pow(vdotr, exponent)).normalized();
}

inline Vector3f diffuse(Vector3f &normal)
{
    Vector3f p;
    float l = 1.0;

    do {
        p = 2.0 * Vector3f(prng_c(0,1), prng_c(0,1), prng_c(0,1)) - Vector3f(1,1,1);
        l = (p * p).length();
        l *= l;
    } while (l >= 1.0);

    p += normal;
    p.normalize();

    return p;
}

	} /* namespace Sample */
} /* namespace NMath */

#endif /* NMATH_SAMPLE_INL_INCLUDED */
