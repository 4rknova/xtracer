/*

    This file is part of the libnmath.

    plane.h
    Plane

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

#ifndef NMATH_PLANE_H_INCLUDED
#define NMATH_PLANE_H_INCLUDED

#include "defs.h"

#include "precision.h"
#include "vector.h"
#include "geometry.h"
#include "ray.h"

namespace NMath {
	
#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */
	
struct plane_t
{
    vec3_t normal;
    scalar_t distance;
};

typedef struct plane_t plane_t;

static inline plane_t plane_pack(vec3_t normal, scalar_t distance);

#ifdef __cplusplus
}	/* __cplusplus */

#define NMATH_PLANE_DEFAULT_DISTANCE 1.0

class Plane: public Geometry
{
	public:
		Plane();
		
		bool intersection(const Ray &ray, IntInfo* i_info) const;   
		void calc_aabb();

		Vector3f normal;
		scalar_t distance;
};

#endif	/* __cplusplus */

} /* namespace NMath */

#include "plane.inl"

#endif /* NMATH_PLANE_H_INCLUDED */
