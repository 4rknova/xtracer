/*

    This file is part of the libnmath.

    plane.cc
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
#include <iostream>
#include <math.h>
#include "plane.h"

#include "precision.h"
#include "vector.h"
#include "intinfo.h"

namespace NMath {

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

#ifdef __cplusplus
}

Plane::Plane()
	: Geometry(GEOMETRY_PLANE), 
	  distance(NMATH_PLANE_DEFAULT_DISTANCE)
{}

// algebraic solution
bool Plane::intersection(const Ray &ray, IntInfo* i_info) const
{
	// check if the ray is travelling parallel to the plane.
	// if the ray is in the plane then we ignore it.
	double n_dot_dir = dot(normal, ray.direction);

	if (fabs(n_dot_dir) < EPSILON) 
		return false;
	
	Vector3f v = Vector3f(nmath_abs(normal.x), nmath_abs(normal.y), nmath_abs(normal.z)) * distance;

	Vector3f vorigin = v - ray.origin;

	double n_dot_vo = dot(vorigin, normal);
	
	double t = n_dot_vo / n_dot_dir; 

	if (t < EPSILON) 
		return false;

	if (i_info) 
	{
		i_info->t = t;
		i_info->point = ray.origin + ray.direction * t;
		i_info->normal = normal;

		// Texture coordinates.
		Vector3f n = normal.normalized();
		Vector3f uvec = Vector3f(n.y, n.z, -n.x);
		Vector3f vvec = cross(uvec, n);
		scalar_t tu = dot(uvec, (v+i_info->point)) * uv_scale.x;
		scalar_t tv = dot(vvec, (v+i_info->point)) * uv_scale.y;
		if (tu > 1.f) tu -= (float)(int)tu;
		if (tv > 1.f) tv -= (float)(int)tv;
		if (tu < -1.f) tu -= (float)(int)tu;
		if (tv < -1.f) tv -= (float)(int)tv;
		if (tu < 0.f) tu = 1.f + tu;
		if (tv < 0.f) tv = 1.f + tv;

		i_info->texcoord = Vector2f(tu, tv);

		i_info->geometry = this;
	}

	return true;
}

void Plane::calc_aabb()
{
	// The plane is infoinite so the bounding box is infinity as well
	aabb.max = Vector3f(INFINITY, INFINITY, INFINITY);
	aabb.min = -aabb.max;
}

#endif	/* __cplusplus */

} /* namespace NMath */
