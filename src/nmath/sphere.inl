/*

    This file is part of the nemesis math library.

    sphere.h
    Sphere inline functions

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

#ifndef LIBNMATH_SPHERE_INL_INCLUDED
#define LIBNMATH_SPHERE_INL_INCLUDED

#ifndef LIBNMATH_SPHERE_H_INCLUDED
    #error "sphere.h must be included before sphere.inl"
#endif /* LIBNMATH_SPHERE_H_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

static inline sphere_t sphere_pack(vec3_t origin, scalar_t radius)
{
	sphere_t s;
	s.origin = origin;
	s.radius = radius;
	return s;
}

#ifdef __cplusplus
}
#endif	/* __cplusplus */

#endif /* LIBNMATH_SPHERE_INL_INCLUDED */
