/*

    This file is part of xtracer.

    geometry.hpp
    Geometry class

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

#ifndef XTRACER_GEOMETRY_HPP_INCLUDED
#define XTRACER_GEOMETRY_HPP_INCLUDED

enum GEOMETRY_TYPE
{
	GEOMETRY_TYPE_SPHERE,	/* Sphere */
	GEOMETRY_TYPE_PLANE,	/* Plane */
	GEOMETRY_TYPE_TRIANGLE	/* Triangle */
};

#include <string>
#include <nmath/vector.h>
#include <nmath/ray.h>

class Geometry 
{
	public:
		Geometry(GEOMETRY_TYPE t);
		~Geometry();

		void *get();					/* Return a pointer to the structure */

		real_t collision(const Ray &ray);		/* Returns collision with ray */

		Vector3 normal_at(Ray &ray, real_t scale);
										/* Returns the normal at the point of collision  */

		const GEOMETRY_TYPE type;		/* The geometry's type */

		std::string material;			/* The associated material */

	private:
		void *m_p_geometry;				/* Pointer to the actual structure */
};

#endif /* XTRACER_GEOMETRY_HPP_INCLUDED */
