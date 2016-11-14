/*

    This file is part of the libnmath.

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

#ifndef NMATH_SPHERE_H_INCLUDED
#define NMATH_SPHERE_H_INCLUDED

#include <nmath/precision.h>
#include <nmath/vector.h>

#include "defs.h"

#include "geometry.h"
#include "ray.h"

namespace NMath {

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

struct sphere_t
{
    vec3_t origin;
    scalar_t radius;
};

typedef struct sphere_t sphere_t;

static inline sphere_t sphere_pack(vec3_t origin, scalar_t radius);

#ifdef __cplusplus
}	/* __cplusplus */

#define NMATH_SPHERE_DEFAULT_RADIUS 1.0

class Sphere: public Geometry
{
    public:
        Sphere();
        Sphere(const Vector3f &org, scalar_t rad);

		bool intersection(const Ray &ray, IntInfo* i_info) const;
		void calc_aabb();

        Vector3f point_sample() const;
        Ray ray_sample() const;

        Vector3f origin;
        scalar_t radius;
};

#endif	/* __cplusplus */

} /* namespace NMath */

#include "sphere.inl"

#endif /* NMATH_SPHERE_H_INCLUDED */
