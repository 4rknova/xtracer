/*

    This file is part of xtracer.

    material.hpp
    Material class

    Copyright (C) 2010, 2011
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

#ifndef XTRACER_MATERIAL_HPP_INCLUDED
#define XTRACER_MATERIAL_HPP_INCLUDED

#include <nmath/intinfo.h>
#include <nimg/color.hpp>
#include "camera.hpp"
#include "light.hpp"
#include "phong.hpp"
#include "lambert.hpp"

using NMath::IntInfo;
using NImg::ColorRGBf;

enum MATERIAL_TYPE
{
	MATERIAL_LAMBERT,		// Lambert
	MATERIAL_PHONG,			// Phong
	MATERIAL_BLINNPHONG		// Blinn-Phong
};

class Material 
{
	public:
		Material();
		~Material();

		// shader
		ColorRGBf shade(const Camera *cam, const Light *light, ColorRGBf &texcolor, const IntInfo &info);
	
		// properties
		ColorRGBf ambient;		// ambient intensity
		ColorRGBf diffuse;	   	// diffuse intensity
		ColorRGBf specular;		// specular intensity
		
		scalar_t kspec;       	// specular constant
		scalar_t kdiff;       	// diffuse constant
		scalar_t ksexp;       	// specular exponential
		scalar_t roughness;		// roughness (Ideally equal to exponent)

		scalar_t reflectance;	// reflectance
		scalar_t transparency;	// transparency
		scalar_t ior;			// index of refraction

		MATERIAL_TYPE type;
};

#endif /* XTRACER_MATERIAL_HPP_INCLUDED */
