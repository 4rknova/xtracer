/*

    This file is part of the nemesis math library.

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

#ifndef LIBNMATH_PLANE_H_INCLUDED
#define LIBNMATH_PLANE_H_INCLUDED

#include "precision.h"
#include "vector.h"
#include "geometry.h"
#include "ray.h"

typedef struct
{
    vec3_t normal;
    scalar_t distance;
} plane_t;

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

static inline plane_t plane_pack(vec3_t normal, scalar_t distance);

#ifdef __cplusplus
}	/* __cplusplus */

#define NMATH_PLANE_DEFAULT_DISTANCE 1.0

class Plane: public Geometry
{
	public:
		Plane();
		Plane(const Vector3 &norm, double distance);
		
		bool intersection(const Ray &ray, IntInfo* i_info) const;   
		void calc_aabb();

		Vector3 normal;
		scalar_t distance; 
};

#endif	/* __cplusplus */

#include "plane.inl"

#endif /* LIBNMATH_PLANE_H_INCLUDED */
