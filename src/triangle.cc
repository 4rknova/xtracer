/*

    This file is part of the libnmath.

    triangle.cc
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

#include "defs.h"
#include "precision.h"
#include "vector.h"
#include "intinfo.h"
#include "triangle.h"

namespace NMath {

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

#ifdef __cplusplus
}

Triangle::Triangle()
	:	Geometry(GEOMETRY_TRIANGLE)
{}

bool Triangle::intersection(const Ray &ray, IntInfo* i_info) const
{
	Vector3f normal = calc_normal();

	double n_dot_dir = dot(normal, ray.direction);

	if (fabs(n_dot_dir) < EPSILON) 
		return false; // parallel to the plane

	// translation of v[0] to axis origin
	Vector3f vo_vec = ray.origin - v[0];

	// calc intersection distance
	scalar_t t = -dot(normal, vo_vec) / n_dot_dir;

	if (t < EPSILON)
		return false; // plane in the opposite subspace

	// intersection point ( on the plane ).
	Vector3f pos = ray.origin + ray.direction * t;

	// calculate barycentric
	Vector3f bc = calc_barycentric(pos);
	scalar_t bc_sum = bc.x + bc.y + bc.z;

	// check for triangle boundaries
	if (bc_sum < 1.0 - EPSILON || bc_sum > 1.0 + EPSILON)
	{
		return false;
	}

	if (i_info)
	{
		i_info->t = t;
		i_info->point = pos;
		i_info->geometry = this;

		// Texcoords
		Vector2f texcoord = tc[0] * bc.x + tc[1] * bc.y + tc[2] * bc.z;
		i_info->texcoord = texcoord;
		
		// Normal
		Vector3f pn = n[0] * bc.x + n[1] * bc.y + n[2] * bc.z;
		i_info->normal = pn.length() ? pn : normal;
	}

	return true;
}

void Triangle::calc_aabb()
{
	aabb.max = Vector3f(-INFINITY, -INFINITY, -INFINITY);
	aabb.min = Vector3f( INFINITY,  INFINITY,  INFINITY);

	for(unsigned int i=0; i<3; i++)
	{
		Vector3f pos = v[i];
		if(pos.x < aabb.min.x) aabb.min.x = pos.x;
		if(pos.y < aabb.min.y) aabb.min.y = pos.y;
		if(pos.z < aabb.min.z) aabb.min.z = pos.z;

		if(pos.x > aabb.max.x) aabb.max.x = pos.x;
		if(pos.y > aabb.max.y) aabb.max.y = pos.y;
		if(pos.z > aabb.max.z) aabb.max.z = pos.z;
	}
}

Vector3f Triangle::calc_normal() const
{
	Vector3f v1 = v[1] - v[0];
	Vector3f v2 = v[2] - v[0];

	return (cross(v1, v2)).normalized();
}

Vector3f Triangle::calc_barycentric(const Vector3f &p) const
{
	Vector3f bc(0.0f, 0.0f, 0.0f);

	Vector3f v1 = v[1] - v[0];
	Vector3f v2 = v[2] - v[0];
	Vector3f xv1v2 = cross(v1, v2);

	Vector3f norm = xv1v2.normalized();

	scalar_t area = fabs(dot(xv1v2, norm)) * 0.5;

	if(area < EPSILON)
	{
		return bc;
	}

	Vector3f pv0 = v[0] - p;
	Vector3f pv1 = v[1] - p;
	Vector3f pv2 = v[2] - p;

	// calculate the area of each sub-triangle
	Vector3f x12 = cross(pv1, pv2);
	Vector3f x20 = cross(pv2, pv0);
	Vector3f x01 = cross(pv0, pv1);

	scalar_t a0 = fabs(dot(x12, norm)) * 0.5;
	scalar_t a1 = fabs(dot(x20, norm)) * 0.5;
	scalar_t a2 = fabs(dot(x01, norm)) * 0.5;
	
	bc.x = a0 / area;
	bc.y = a1 / area;
	bc.z = a2 / area;

	return bc;
}

#endif	/* __cplusplus */

} /* namespace NMath */
