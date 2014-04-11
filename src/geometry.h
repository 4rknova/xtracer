/*

    This file is part of the libnmath.

    geometry.h
    Geometry

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

#ifndef NMATH_GEOMETRY_H_INCLUDED
#define NMATH_GEOMETRY_H_INCLUDED

#include "vector.h"
#include "aabb.h"
#include "ray.h"

namespace NMath {

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

enum NMATH_GEOMETRY_TYPE
{
	GEOMETRY_PLANE,
	GEOMETRY_TRIANGLE,
	GEOMETRY_SPHERE,
	
	GEOMETRY_MESH		/* For external objects that might extend geometry */
};

#ifdef __cplusplus
}	/* __cplusplus */

// forward declaration
class IntInfo;

class Geometry
{
    public:
		Geometry(NMATH_GEOMETRY_TYPE t);
		virtual ~Geometry(); 
		virtual bool intersection(const Ray &ray, IntInfo* i_info) const = 0;
		virtual void calc_aabb() = 0;

		NMATH_GEOMETRY_TYPE type;

		BoundingBox3 aabb;

		Vector2f uv_scale;
};

#endif	/* __cplusplus */

} /* namespace NMath */

#endif /* NMATH_GEOMETRY_H_INCLUDED */
