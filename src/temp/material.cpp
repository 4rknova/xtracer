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

#include "matlambert.hpp"

Material::Material(MATERIAL_TYPE t)
	: type(t), m_p_material(NULL)
{
	switch ((int)type)
	{
		case MATERIAL_TYPE_LAMBERT:
			m_p_material = new MatLambert();

		default:
			return;
	}
}

Material::~Material()
{
	switch ((int)type)
	{
		case MATERIAL_TYPE_LAMBERT:
			delete (MatLambert *) m_p_material;

		default:
			return;
	}
}

void *Material::get()
{
	return m_p_material;
}


