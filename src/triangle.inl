/*

    This file is part of the libnmath.

    triangle.h
    Triangle inline functions

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

#ifndef NMATH_TRIANGLE_INL_INCLUDED
#define NMATH_TRIANGLE_INL_INCLUDED

#ifndef NMATH_TRIANGLE_H_INCLUDED
    #error "triangle.h must be included before triangle.inl"
#endif /* NMATH_TRIANGLE_H_INCLUDED */

namespace NMath {

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

static inline triangle_t triangle_pack(vec3_t v0, vec3_t v1, vec3_t v2)
{
	triangle_t t;
	t.v[0] = v0;
	t.v[1] = v1;
	t.v[2] = v2;
	return t;
}

#ifdef __cplusplus
}
#endif	/* __cplusplus */

} /* namespace NMath */

#endif /* NMATH_TRIANGLE_INL_INCLUDED */
