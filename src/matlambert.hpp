/*

    This file is part of xtracer.

    matlambert.hpp
    MatLambert class

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

#ifndef XTRACER_MAT_LAMBERT_HPP_INCLUDED
#define XTRACER_MAT_LAMBERT_HPP_INCLUDED

#include <nmath/vector.h>

#include "material.hpp"
#include "light.hpp"

class MatLambert: public Material
{
	public:
		MatLambert();

		// shade
		Vector3 shade(Light *light, IntInfo &info);

		Vector3 diffuse;	// diffuse intensity
};

#endif /* XTRACER_MAT_LAMBERT_HPP_INCLUDED */
