/*

    This file is part of xtracer.

    material.cpp
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

#include "material.hpp"

Material::Material()
	:
	// Color values
	diffuse(Vector3(1.0, 1.0, 1.0)),
	specular(Vector3(1.0, 1.0, 1.0)),
	ambient(Vector3(1.0, 1.0, 1.0)),
	// Constants
	kspec(0.0),
	kdiff(1.0),
	ksexp(60),
	// Ratios
	reflectance(0.0),
	transparency(0.0),
	// Index of refraction
	ior(1.5),
	// Material type
	type(MATERIAL_LAMBERT)
{}

Material::~Material()
{}

Vector3 Material::shade(const Camera *cam, const Light *light, const IntInfo &info)
{
	switch(type)
	{
		case MATERIAL_LAMBERT:
			return lambert(
				light->position, 
				&info, 
				light->intensity, 
				diffuse);

		case MATERIAL_PHONG:
			return phong(
				cam->position, 
				light->position, 
				&info, 
				light->intensity, 
				kspec, kdiff, ksexp, 
				diffuse, 
				specular);

		case MATERIAL_BLINNPHONG:
			return blinn_phong(
				cam->position, 
				light->position, 
				&info, 
				light->intensity, 
				kspec, kdiff, ksexp, 
				diffuse, 
				specular);
	}

	// This should never happen
	return Vector3(0, 0, 0);
}

