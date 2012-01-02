/*

    This file is part of the nemesis math library.

    ray.cc
    Ray

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

#include "ray.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

#ifdef __cplusplus
}

Ray::Ray(const Vector3 &org, const Vector3 &dir)
    : origin(org), direction(dir.normalized())
{}

Ray::Ray()
	: origin(Vector3(0,0,0)), direction(Vector3(0,0,1))
{}

#endif	/* __cplusplus */
