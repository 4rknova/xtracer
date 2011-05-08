/*

    This file is part of xtracer.

    material.hpp
    Material class

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

#ifndef XTRACER_MATERIAL_HPP_INCLUDED
#define XTRACER_MATERIAL_HPP_INCLUDED

#include <nmath/vector.h>
#include <nmath/intinfo.h>

#include "camera.hpp"
#include "light.hpp"

class Material 
{
	public:
		Material();
		virtual ~Material();

		// shade
		virtual Vector3 shade(const Camera *cam, const Light *light, const IntInfo &info) = 0;
		
		Vector3 diffuse;   	// diffuse intensity
		real_t reflectance;
		real_t transparency;
		real_t ior;
};

#endif /* XTRACER_MATERIAL_HPP_INCLUDED */
