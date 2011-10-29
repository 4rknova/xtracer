/*

    This file is part of xtracer. 

    vertex.hpp
    Vertex

    Copyright (C) 2008, 2010, 2011
    Papadopoulos Nikolaos

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General
    Public License along with this program; if not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301 USA

*/

#ifndef LIBNMATH_VERTEX_HPP_INCLUDED
#define LIBNMATH_VERTEX_HPP_INCLUDED

#include <nmath/precision.h>
#include <nmath/vector.h>

struct Vertex {
	Vector3 position, normal, tangent;
	Vector2 texture_uv;
};

#endif /* LIBNMATH_VERTEX_HPP_INCLUDED */
