/*

    This file is part of xtracer.

    light.cpp
    Light class

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

#include "light.hpp"

#include <nmath/precision.h>

Light::Light()
	: m_p_position(Vector3(0,0,0))
{}

Light::Light(Vector3 &position)
	: m_p_position(position)
{}

Vector3 Light::position()
{
	return m_p_position;
}

Vector3 Light::set_position(Vector3 &position)
{
	return m_p_position = position;
}
