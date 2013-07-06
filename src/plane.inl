/*

    This file is part of the libnmath.

    plane.h
    Plane inline functions

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

#ifndef NMATH_PLANE_INL_INCLUDED
#define NMATH_PLANE_INL_INCLUDED

#ifndef NMATH_PLANE_H_INCLUDED
    #error "plane.h must be included before plane.inl"
#endif /* NMATH_PLANE_H_INCLUDED */

namespace NMath {

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

static inline plane_t plane_pack(vec3_t normal, scalar_t distance)
{
	plane_t s;
	s.normal = normal;
	s.distance = distance;
	return s;
}

#ifdef __cplusplus
}
#endif	/* __cplusplus */

} /* namespace NMath */

#endif /* NMATH_PLANE_INL_INCLUDED */
