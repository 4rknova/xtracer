/*

    This file is part of nemesis math library.

    geometry.cpp
    Geometry

    Copyright (C) 2010, 2011
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

#include "geometry.hpp"

#include <nmath/vector.h>
#include <nmath/sphere.h>

Geometry::Geometry(GEOMETRY_TYPE t)
	: type(t), m_p_geometry(NULL)
{
	switch ((int)type)
	{
		case GEOMETRY_TYPE_SPHERE:
			m_p_geometry = new Sphere;

		default:
			return;
	}

}

Geometry::~Geometry()
{
	switch ((int)type)
	{
		case GEOMETRY_TYPE_SPHERE:
			delete (Sphere *) m_p_geometry;

		default:
			return;
	}
}

void *Geometry::get()
{
	return m_p_geometry;
}

#include <nmath/precision.h>
#include <nmath/intersection.h>

real_t Geometry::collision(const Ray &ray)
{

	Ray r = ray;
	Sphere s;

	switch ((int)type)
	{
		case GEOMETRY_TYPE_SPHERE:
			s = Sphere(((Sphere*)m_p_geometry)->origin, ((Sphere*)m_p_geometry)->radius);
			return intersect_ray_sphere(r, s);
	    default:
			return NM_INFINITY;
	}
}

Vector3 Geometry::normal_at(Ray &ray, real_t scale)
{
	Vector3 normal;

	switch ((int)type)
	{
		case GEOMETRY_TYPE_SPHERE:
			Vector3 dir(ray.direction - ray.origin);

			break;
	}

	return normal;
}
