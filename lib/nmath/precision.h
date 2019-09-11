/*

	This file is part of libnmath.

	precision.h
	Precision

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

#ifndef NMATH_PRECISION_H_INCLUDED
#define NMATH_PRECISION_H_INCLUDED

#include <float.h>

#ifdef __cplusplus
	#include <cmath>
#else
	#include <math.h>
#endif	/* __cplusplus */

namespace NMath {

#ifdef __cplusplus
	extern "C" {
#endif	/* __cplusplus */

/* Floating point precision */
#ifdef MATH_SINGLE_PRECISION
	#define SCALAR_T_MAX FLT_MAX

	typedef float scalar_t;

	#define nmath_sqrt	sqrtf
	#define nmath_abs	fabs

	#define nmath_sin	sinf
	#define nmath_cos	cosf
	#define nmath_tan	tanf
	#define nmath_asin	asinf
	#define nmath_acos	acosf
	#define nmath_atan	atanf
	#define nmath_atan2	atan2
	#define nmath_pow	pow

#else
	#define SCALAR_T_MAX DBL_MAX

	typedef double scalar_t;

	#define nmath_sqrt  sqrt
	#define nmath_abs   fabs

	#define nmath_sin	sin
	#define nmath_cos	cos
	#define nmath_tan	tan
	#define nmath_asin	asin
	#define nmath_acos	acos
	#define nmath_atan	atan
	#define nmath_atan2	atan2
	#define nmath_pow	pow

#endif /* MATH_SINGLE_PRECISION */

/* Infinity */
#ifndef INFINITY
	#define INFINITY SCALAR_T_MAX
#endif /* INFINITY */

#ifndef EPSILON
	#define EPSILON		1E-8
#endif /* EPSILON */

/* Useful scalar_t values used in comparisons */
const scalar_t ERROR_MARGIN	   = EPSILON;
const scalar_t SCALAR_MEDIUM   = 1.e-2;   /* 0.01 */
const scalar_t SCALAR_SMALL    = 1.e-4;   /* 0.0001 */
const scalar_t SCALAR_XSMALL   = 1.e-6;   /* 0.000001 */
const scalar_t SCALAR_XXSMALL  = 1.e-8;   /* 0.00000001 */
const scalar_t SCALAR_XXXSMALL = 1.e-12;  /* 0.000000000001 */

/* PI */
const scalar_t PI_DOUBLE	= 6.283185307179586232;
const scalar_t PI			= 3.14159265358979323846;
const scalar_t PI_HALF		= 1.57079632679489661923;
const scalar_t PI_QUARTER	= 0.78539816339744830962;

/* EULER_E */
const scalar_t EULER_E = 2.7182818284590452354;

const scalar_t LN2	= 0.69314718055994530942;
const scalar_t LN10	= 2.30258509299404568402;

/* RADIAN */
const scalar_t RADIAN = 0.017453292519943;

#ifdef __cplusplus
	}   /* extern "C" */
#endif

inline scalar_t max  (scalar_t x, scalar_t y)                         { return (x > y ? x : y);                   }
inline scalar_t max3 (scalar_t x, scalar_t y, scalar_t z)             { return (max(max(x,y),z));                 }
inline scalar_t max4 (scalar_t x, scalar_t y, scalar_t z, scalar_t w) { return (max(max(max(x,y),z),w));          }
inline scalar_t min  (scalar_t x, scalar_t y)                         { return (y > x ? x : y);                   }
inline scalar_t min3 (scalar_t x, scalar_t y, scalar_t z)             { return (min(min(x,y),z));                 }
inline scalar_t min4 (scalar_t x, scalar_t y, scalar_t z, scalar_t w) { return (min(min(min(x,y),z),w));          }
inline scalar_t sign (scalar_t x)                                     { return (x < 0 ? -1 : ( x > 0 ? 1 : 0));   }

} /* namespace NMath */

// Check for standard definitions
#ifndef M_PI
    #define M_PI NMath::PI
#endif /* M_PI */

#ifndef M_E
    #define M_E	NMath::EULER_E
#endif /* M_E */

#endif /* NMATH_PRECISION_H_INCLUDED */
