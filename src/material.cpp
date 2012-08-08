/*

    This file is part of xtracer.

    material.cpp
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

#include "material.hpp"

Material::Material()
	: ambient(ColorRGBf(1.0, 1.0, 1.0)),
	  diffuse(ColorRGBf(1.0, 1.0, 1.0)),
	  specular(ColorRGBf(1.0, 1.0, 1.0)),
	  kspec(0.0),
	  kdiff(1.0),
	  ksexp(60),
	  reflectance(0.0),
	  transparency(0.0),
	  ior(1.5),
	  type(MATERIAL_LAMBERT)
{}

Material::~Material()
{}

ColorRGBf Material::shade(const Camera *cam, const Light *light, ColorRGBf &texcolor, const IntInfo &info)
{
	switch(type)
	{
		case MATERIAL_LAMBERT:
			return lambert(
				light->point_sample(), 
				&info, 
				light->intensity(), 
				diffuse * texcolor);

		case MATERIAL_PHONG:
			return phong(
				cam->position, 
				light->point_sample(), 
				&info, 
				light->intensity(), 
				kspec, kdiff, ksexp, 
				diffuse * texcolor, 
				specular);

		case MATERIAL_BLINNPHONG:
			return blinn_phong(
				cam->position, 
				light->point_sample(), 
				&info, 
				light->intensity(), 
				kspec, kdiff, ksexp, 
				diffuse * texcolor, 
				specular);
	}

	// This should never happen
	return ColorRGBf(0, 0, 0);
}

