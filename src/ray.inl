/*

    This file is part of the libnmath.

    ray.inl
    Ray inline functions

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

#ifndef NMATH_RAY_INL_INCLUDED
#define NMATH_RAY_INL_INCLUDED

#ifndef NMATH_RAY_H_INCLUDED
    #error "ray.h must be included before ray.inl"
#endif /* NMATH_RAY_H_INCLUDED */

namespace NMath {

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

static inline ray_t ray_pack(vec3_t origin, vec3_t direction)
{
	ray_t r;
	r.origin = origin;
	r.direction = vec3_normalize(direction);
	return r;
}

#ifdef __cplusplus
}
#endif	/* __cplusplus */

} /* namespace NMath */

#endif /* NMATH_RAY_INL_INCLUDED */
