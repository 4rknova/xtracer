/*

    This file is part of the nemesis math library.

    sphere.h
    Sphere

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

#ifndef LIBNMATH_SPHERE_H_INCLUDED
#define LIBNMATH_SPHERE_H_INCLUDED

#include "precision.h"
#include "vector.h"
#include "geometry.h"
#include "ray.h"

typedef struct
{
    vec3_t origin;
    scalar_t radius;
} sphere_t;

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

static inline sphere_t sphere_pack(vec3_t origin, scalar_t radius);

#ifdef __cplusplus
}	/* __cplusplus */

#define NMATH_SPHERE_DEFAULT_RADIUS 1.0

class Sphere: public Geometry
{
    public:
        Sphere();
        Sphere(const Vector3 &org, scalar_t rad);

		bool intersection(const Ray &ray, IntInfo* i_info) const;
		void calc_aabb();

        Vector3 origin;
        scalar_t radius;
};

#endif	/* __cplusplus */

#include "sphere.inl"

#endif /* LIBNMATH_SPHERE_H_INCLUDED */
