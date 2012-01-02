/*

    This file is part of the nemesis math library.

    aabb.cc
    Bounding box functions

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

#include "aabb.h"

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

#ifdef __cplusplus
}

/* BoundingBox2 class */
BoundingBox2::BoundingBox2(){}

BoundingBox2::BoundingBox2(const Vector2& a, const Vector2& b)
{
    min=Vector2( (a.x<=b.x)? a.x : b.x, (a.y<=b.y)? a.y : b.y );
    min=Vector2( (a.x>=b.x)? a.x : b.x, (a.y>=b.y)? a.y : b.y );
}

/* BoundingBox3 class */
BoundingBox3::BoundingBox3(){}

BoundingBox3::BoundingBox3(const Vector3& a, const Vector3& b)
{
    min=Vector3( (a.x<=b.x)? a.x : b.x, (a.y<=b.y)? a.y : b.y, (a.z<=b.z)? a.z : b.z );
    min=Vector3( (a.x>=b.x)? a.x : b.x, (a.y>=b.y)? a.y : b.y, (a.z>=b.z)? a.z : b.z );
}

/* 	
	ray - axis aligned bounding box intersection test based on:
	"An Efficient and Robust Ray-Box Intersection Algorithm",
	Amy Williams, Steve Barrus, R. Keith Morley, and Peter Shirley
	Journal of graphics tools, 10(1):49-54, 2005
*/

#include <iostream>
bool BoundingBox3::intersection(const Ray &ray) const
{
	if (ray.origin > min && ray.origin < max)
	{
		return true;
	}

	Vector3 aabb[2] = {min, max};
	static const double t0 = 0.0;

	int xsign = (int)(ray.direction.x < 0.0);
	double invdirx = 1.0 / ray.direction.x;

	double tmin = (aabb[xsign].x - ray.origin.x) * invdirx;
	double tmax = (aabb[1 - xsign].x - ray.origin.x) * invdirx;

	int ysign = (int)(ray.direction.y < 0.0);
	double invdiry = 1.0 / ray.direction.y;
	double tymin = (aabb[ysign].y - ray.origin.y) * invdiry;
	double tymax = (aabb[1 - ysign].y - ray.origin.y) * invdiry;
	
	if ((tmin > tymax) || (tymin > tmax))
	{
		return false;
	}

	if (tymin > tmin) tmin = tymin;
	if (tymax < tmax) tmax = tymax;

	int zsign = (int)(ray.direction.z < 0.0);
	double invdirz = 1.0 / ray.direction.z;
	double tzmin = (aabb[zsign].z - ray.origin.z) * invdirz;
	double tzmax = (aabb[1 - zsign].z - ray.origin.z) * invdirz;

	if ((tmin > tzmax) || (tzmin > tmax))
	{
		return false;
	}

	if (tzmin > tmin) tmin = tzmin;
	if (tzmax < tmax) tmax = tzmax;

	return (tmax > t0);
}

#endif	/* __cplusplus */
