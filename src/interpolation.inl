/*

	This file is part of libnmath.

	interpolation.inl
	Interpolation methods

	Copyright (C) 2008, 2010 - 2012
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

#ifndef NMATH_INTERPOLATION_INL_INCLUDED
#define NMATH_INTERPOLATION_INL_INCLUDED

#ifndef NMATH_INTERPOLATION_H_INCLUDED
    #error "interpolation.h must be included before interpolation.inl"
#endif /* NMATH_INTERPOLATION_H_INCLUDED */

#include "precision.h"
#include "defs.h"

namespace NMath {
	namespace Interpolation {

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

static inline scalar_t smoothstep(const scalar_t edge0, const scalar_t edge1, const scalar_t x)
{
	/*
		This is essentially cubic Hermite interpolation.
		Formula: (3 * (t^2)) - (2 * (t^3)
	*/

	/* Scale, bias and saturate x to the [0-1] range */
	scalar_t y = (x - edge0) / (edge1 - edge0);

	if (y < 0) y = 0;
	else if (y > 1) y = 1;

	/* Evaluate polynomial */
	return y * y * (3 - 2 * y);
}

/* Ken Perlin's suggested alternative which has zero 1st and 2nd order derivatives at t = 0 and t = 1 */
static inline scalar_t smoothstep_perlin(const scalar_t edge0, const scalar_t edge1, const scalar_t x)
{
	/*
		This is essentially Hermite interpolation.
		Formula (6 * (t^5)) - (15 * (t^4)) + (10 * (t^3))
	*/

	/* Scale, and saturate x to the [0-1] range */
	scalar_t y = (x - edge0) / (edge1 - edge0);

	if (y < 0) y = 0;
	else if (y > 1) y = 1;

	/* Evaluate polynomial */
	return y * y * y * (y * (y * 6 - 15) + 10);
}

static inline scalar_t step(const scalar_t a, const scalar_t b, const scalar_t t)
{
    return (t < 0.5) ? a : b;
}

static inline scalar_t linear(const scalar_t a, const scalar_t b, const scalar_t t)
{
    return  a + (b - a) * t;
}

static inline scalar_t cosine(const scalar_t a, const scalar_t b, const scalar_t t)
{
	/*
        First we turn the p value into an angle to sample from the
        cosine wave and sample from the wave but converting the
        scale run between 0 and 1 instead of the wave's usual -1 to 1.
        Lastly we perform a normal linear interpolation, but using the
        value from the cosine wave instead of the value of the given p
    */

    scalar_t t2 = (1 - nmath_cos(t * PI)) / 2;
    return (a * (1 - t2) + b * t2);
}

static inline scalar_t acceleration(const scalar_t a, const scalar_t b, const scalar_t t)
{
    scalar_t p2 = t * t;
    return  (a * (1 - p2)) + (b * p2);
}

static inline scalar_t deceleration(const scalar_t a, const scalar_t b, const scalar_t t)
{
    scalar_t rt = 1 - t;
    scalar_t t2 = 1 - (rt * rt);
    return  (a * (1 - t2)) + (b * t2);
}

static inline scalar_t cubic(const scalar_t a, const scalar_t b, const scalar_t c, const scalar_t d, const scalar_t t)
{
	/*
		Cubic interpolation is the simplest method that offers true continuity. 
		It requires more than just the two endpoints of the segment but also the two points on 
		either side of them. So the function requires 4 points in all labeled a, b, c, d, in the 
		code below. t still behaves the same way for interpolating between the segment b to c. 
	*/

    scalar_t P = (d - c) - (a - b);
	scalar_t Q = (a - b) - P;
	scalar_t R = c - a;
	scalar_t S = b;

	scalar_t t2 = t * t;
    scalar_t t3 = t2 * t;

	return (P * t3) + (Q * t2) + (R * t) + S;
}

static inline scalar_t hermite(const scalar_t tang1, const scalar_t a, const scalar_t b, const scalar_t tang2, const scalar_t t)
{
	/*
		Notes:
		This is valid for  0 < t < 1
		The fuctionality can easily be generalized for arbitrary space (m, n).
		However considering the purpose of this library we will mostly use it in the
		[0-1] space and such a generalization would introduce unnecessary computations.

		a     : the startpoint of the curve.
		b     : he endpoint of the curve.
		t     : 0 - 1.
		tang1 : the tangent (e.g. direction and speed) to how the curve leaves the startpoint.
		tang2 : the tangent (e.g. direction and speed) to how the curves meets the endpoint.

		Hermite basis functions:

		h1(t) =  2t^3 - 3t^2 + 1
		h2(t) = -2t^3 + 3t^2
		h3(t) =   t^3 - 2t^2 + t
		h4(t) =   t^3 -  t^2

		Final formula: (h1 * a) + (h2 * b) + (h3 * tang1) + (h4 * tang2)

		h3 and h4 are applied to the tangents to make sure that the curve bends in the desired 
		direction at the start and endpoint.

		Matrix notation:
		Vector T: The interpolation-point and it's powers up to 3:
		Vector C: The parameters of the hermite curve:
		Matrix H: The matrix form of the 4 hermite polynomials:

	   	      | t^3 |            | a     |                |  2  -2   1   1 |
		 T =  | t^2 |       C =  | b     |           H =  | -3   3  -2  -1 |
			  | t^1 |            | tang1 |                |  0   0   1   0 |
			  | 1   |            | tang2 |                |  1   0   0   0 |

		To calculate a point Y on the curve you build the Vector T, multiply it with the matrix H 
		and then multiply with C.
		
		Y = T * H * C
	*/

	scalar_t t2 = t * t;
    scalar_t t3 = t2 * t;

    scalar_t h1 = (2 * t3) - (3 * t2) + 1;          /* calculate basis function 1 */
    scalar_t h2 = 1 - h1;                           /* calculate basis function 2 */
    scalar_t h3 = t3 - (2 * t2) + t;                /* calculate basis function 3 */
    scalar_t h4 = t3 - t2;                          /* calculate basis function 4 */

    return (h1 * a) + (h2 * b) + (h3 * tang1) + (h4 * tang2); /* multiply and sum all functions together */
}

static inline scalar_t cardinal(const scalar_t a, const scalar_t b, const scalar_t c, const scalar_t d, const scalar_t p, const scalar_t t)
{
	/*
		Notes:
		Cardinal splines are just a subset of the hermite curves.
		They don't need the tangent points because they will be calculated from the control
		points. We'll lose some of the flexibility of the hermite curves, but as a tradeoff 
		the curves will be much easier to use.

		We are interpolating between b and c.
		a and d are control points.
		
		The formula for the tangents of cardinal splines is:

		Tangenti = p * ( Xi - Xj )

		where
			Xn : nth point (in the above: j = i - 2).
			p  : constant which affects the tightness of the curve.
	*/

	scalar_t t2 = t * t;
    scalar_t t3 = t2 * t;

    scalar_t h1 = (2 * t3) - (3 * t2) + 1;          /* calculate basis function 1 */
    scalar_t h2 = 1 - h1;                           /* calculate basis function 2 */
    scalar_t h3 = t3 - (2 * t2) + t;                /* calculate basis function 3 */
    scalar_t h4 = t3 - t2;                          /* calculate basis function 4 */

	scalar_t tang1 = p * (c - a);
	scalar_t tang2 = p * (d - b);
	
    return (h1 * b) + (h2 * c) + (h3 * tang1) + (h4 * tang2); /* multiply and sum all functions together */
}

static inline scalar_t catmullrom(const scalar_t a, const scalar_t b, const scalar_t c, const scalar_t d, const scalar_t t)
{
    /*
        Notes:
        CatmullRoms are cardinals with a tension (p) of 0.5
		
		I have verified the correctness of this method both mathematically and 
		by using the cardinal. Both functions yielded the same results. This 
		implementation is chosen due to lower computational cost.

		ATTENTION: This algorithm is NOT proper for general cardinals, don't
		let the p (0.5) multiplication fool you. It is a trick to save a few
		multiplications in h. The h coefficients used below are only applicable
		to Catmull-Rom splines.
    */
    scalar_t P = -a + (3 * (b - c)) + d;
    scalar_t Q = (2 * a) - (5 * b) + (4 * c) - d;
    scalar_t R = c - a;
    scalar_t S = 2 * b;

   	scalar_t t2 = t * t;
    scalar_t t3 = t2 * t;

    return 0.5 * ((P * t3) + (Q * t2) + (R * t) + S);
}

static inline scalar_t bezier_quadratic(const scalar_t a, const scalar_t b, const scalar_t c, const scalar_t t)
{
	/* DeCasteljau */
	scalar_t ab = linear(a, b, t);
	scalar_t bc = linear(b, c, t);
	return linear( ab, bc, t);
}

static inline scalar_t bezier_cubic(const scalar_t a, const scalar_t b, const scalar_t c, const scalar_t d, const scalar_t t)
{
	/* DeCasteljau */
	scalar_t ab  = linear(a, b, t);
	scalar_t bc  = linear(b, c, t);
	scalar_t cd  = linear(c, d, t);
	scalar_t abc = linear(ab, bc, t);
	scalar_t bcd = linear(bc, cd, t);
	return linear( abc, bcd, t);
}

#ifdef __cplusplus
}   /* extern "C" */
#endif

	} /* namespace Interpolation */
} /* namespace NMath */

#endif /* NMATH_INTERPOLATION_INL_INCLUDED */
