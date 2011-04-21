/*

    This file is part of xtracer.

    object.cpp
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

#include "object.hpp"

Object::Object(GEOMETRY_TYPE t)
	: type(t)
{
	init();
}

Object::~Object()
{
	deinit();
}

#include <nmath/sphere.h>
#include <nmath/plane.h>

int Object::init()
{
	switch ((int)type)
	{
		case GEOMETRY_SPHERE:
			geometry = new Sphere;
			break;
		case GEOMETRY_PLANE:
			geometry = new Plane;
			break;
		default: // This will never happen, I am just paranoid.
			return 1;
	}

	return 0;
}

void Object::deinit()
{
	delete geometry;
}
