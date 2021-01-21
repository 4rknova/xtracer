/*

    This file is part of the libnmath.

    mutil.inl
    Declares some math utility functions

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

#ifndef NMATH_MUTIL_INL_INCLUDED
#define NMATH_MUTIL_INL_INCLUDED

#ifndef NMATH_MUTIL_H_INCLUDED
    #error "mutil.h must be included before mutil.inl"
#endif /* NMATH_MUTIL_H_INCLUDED */

namespace NMath {

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Inverse square root */
/*
	Notes:
	It is widely known by the implementation provided with the Quake 3 engine source code.
	It uses Newton-Raphson iteration with a very clever first approximation.
	The creator is unknown but traced back as:	John Carmack -> Michael Abrash ->
	Terje Matheson -> Gary Tarollii -> Greg Walsh & Cleve Moler

	A faster alternative is to use SSE rsqrtss
*/
static inline float approx_invsqrt1(const float x)
{
	float xcopy = x;
	float xhalf = 0.5f * xcopy;
	int i = (long)xcopy;
	i = 0x5f3759df - (i >> 1);
	xcopy = (float)i;
	xcopy *= (1.5f - xhalf * xcopy * xcopy);
	return xcopy;
}

static inline float approx_invsqrt2(const float x)
{
	float xcopy = x;
	float xhalf = 0.5f * xcopy;
	int i = (long)xcopy;
	i = 0x5f3759df - (i >> 1);
	xcopy = (float)i;
	xcopy *= (1.5f - xhalf * xcopy * xcopy);
	xcopy *= (1.5f - xhalf * xcopy * xcopy);
	return xcopy;
}

/* Conversion between radians and degrees */
static inline scalar_t degree_to_radian(const scalar_t r)
{
    return (r * 180.0f) / PI;
}

static inline scalar_t radian_to_degree(const scalar_t d)
{
    return d * (PI / 180.0f);
}

/* Clamping */
static inline scalar_t saturate(const scalar_t value)
{
	return value < 0 ? 0 : (value > 1 ? 1 : value);
}

static inline scalar_t clamp_min(const scalar_t value, const scalar_t min)
{
	return value < min ? min : value;
}

static inline scalar_t clamp_max(const scalar_t value, const scalar_t max)
{
	return value > max ? max : value;
}

static inline scalar_t clamp(const scalar_t value, const scalar_t a, const scalar_t b)
{
	if (a > b)
		return clamp(value, b, a);

	return value < a ? a : (value > b ? b : value);
}

/* Check if an integer is a power of 2 */
static inline int is_power_of_2(const int v)
{
	return ( (v > 0) && ((v & (v - 1)) == 0) );
}

#ifdef __cplusplus
}   /* extern "C" */
#endif /* __cplusplus */

} /* namespace NMath */

#endif /* NMATH_MUTIL_INL_INCLUDED */
