/*

    This file is part of xtracer.

    object.hpp
    Object class

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

#ifndef XTRACER_OBJECT_HPP_INCLUDED
#define XTRACER_OBJECT_HPP_INCLUDED

#include <string>
#include <nmath/geometry.h>

enum GEOMETRY_TYPE
{
	GEOMETRY_SPHERE,
	GEOMETRY_PLANE
};

class Object 
{
	public:
		Object(GEOMETRY_TYPE t);
		~Object();

		// The geometry's type
		const GEOMETRY_TYPE type;

		// A pointer to the geometry object
		Geometry *geometry;

		// The material's name
		std::string material;

	private:
		int init();
		void deinit();
};

#endif /* XTRACER_OBJECT_HPP_INCLUDED */
