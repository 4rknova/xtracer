/*

    This file is part of the libnmath.

    triangle.h
    Triangle

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

#ifndef NMATH_TRIANGLE_H_INCLUDED
#define NMATH_TRIANGLE_H_INCLUDED

#include "defs.h"

#include "precision.h"
#include "types.h"
#include "vector.h"
#include "geometry.h"
#include "ray.h"

namespace NMath {

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

struct triangle_t { vec3_t v[3]; };
typedef struct triangle_t triangle_t;

static inline triangle_t triangle_pack(vec3_t v0, vec3_t v1, vec3_t v2);

#ifdef __cplusplus
}	/* __cplusplus */

class Triangle: public Geometry
{
    public:
        Triangle();

		bool intersection(const Ray &ray, IntInfo* i_info) const;
		void calc_aabb();
		Vector3f calc_normal() const;
		Vector3f calc_barycentric(const Vector3f &p) const;

        Vector3f v[3]; // position
        Vector3f n[3]; // normal
		Vector2f tc[3]; // texcoords
};

#endif	/* __cplusplus */

} /* namespace NMath */

#include "triangle.inl"

#endif /* NMATH_TRIANGLE_H_INCLUDED */
