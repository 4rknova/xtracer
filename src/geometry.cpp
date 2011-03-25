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

#include <nmath/sphere.h>

Geometry::Geometry(GEOMETRY_TYPE type)
	: m_p_type(type)
{
	switch ((int)m_p_type)
	{
		case GEOMETRY_TYPE_SPHERE:
			m_p_geometry = new Sphere;

		default:
			return;
	}

}

Geometry::~Geometry()
{
	switch ((int)m_p_type)
	{
		case GEOMETRY_TYPE_SPHERE:
			delete (Sphere *) m_p_geometry;

		default:
			return;
	}
}

GEOMETRY_TYPE Geometry::type()
{
	return m_p_type;
}

void *Geometry::get()
{
	return m_p_geometry;
}

#include <nmath/precision.h>
#include <nmath/intersection.h>

real_t Geometry::collision(Ray &ray)
{

	Ray r = ray;
	Sphere s;

	switch ((int)m_p_type)
	{
		case GEOMETRY_TYPE_SPHERE:
			s = Sphere(((Sphere*)m_p_geometry)->origin, ((Sphere*)m_p_geometry)->radius);
			return intersect_ray_sphere(r, s);
	    default:
			return NM_INFINITY;
	}
}
