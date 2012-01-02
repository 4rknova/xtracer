/*

    This file is part of the nemesis math library.

    intinfo.h
    Intinfo structure 

    Copyright (C) 2011
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

#ifndef LIBNMATH_INTINFO_H_INCLUDED
#define LIBNMATH_INTINFO_H_INCLUDED

#include "vector.h"
#include "precision.h"
#include "geometry.h"

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

#ifdef __cplusplus
}   /* extern "C" */

class IntInfo
{
	public:
		IntInfo();

		Vector3 normal;
		Vector3 point;
		scalar_t t;
		const Geometry* geometry;
};

#endif /* __cplusplus */

#endif /* LIBNMATH_INTINFO_H_INCLUDED */
