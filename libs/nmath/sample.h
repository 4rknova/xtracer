/*

	This file is part of libnmath.

	sample.h
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

#ifndef NMATH_SAMPLE_H_INCLUDED
#define NMATH_SAMPLE_H_INCLUDED

#include "precision.h"
#include "vector.h"

namespace NMath {
	namespace Sample {

#ifdef __cplusplus

inline Vector3f sphere();
inline Vector3f hemisphere(const Vector3f &normal, const Vector3f &direction);
inline Vector3f lobe(const Vector3f &normal, const Vector3f &direction, const scalar_t exponent);

#endif /* __cplusplus */

	} /* namespace Sample */
} /* namespace NMath */

#include "sample.inl"

#endif /* NMATH_SAMPLE_H_INCLUDED */
