/*

    This file is part of xtracer.

    camera.cpp
    Camera class

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

#include "camera.hpp"

#include <nmath/defs.h>
#include <nmath/ray.h>

#define XTRACER_DEFAULT_FOV M_PI/4

XTCamera::XTCamera(XTFramebuffer &fb)
	: m_p_position(Vector3(0,0,0)), m_p_target(Vector3(0,0,0)), m_p_fov(XTRACER_DEFAULT_FOV), m_p_fb(&fb)
{}

XTCamera::XTCamera(Vector3 &position, Vector3 &target, real_t fov, XTFramebuffer &fb)
:  m_p_position(position), m_p_target(target), m_p_fov(fov), m_p_fb(&fb)
{}

Vector3 XTCamera::get_position()
{
	return m_p_position;
}

Vector3 XTCamera::get_target()
{
	return m_p_target;
}

real_t XTCamera::get_fov()
{
	return m_p_fov;
}

Vector3 XTCamera::set_position(Vector3 &position)
{
	return m_p_position = position;
}

Vector3 XTCamera::set_target(Vector3 &target)
{
	return m_p_target = target;
}

real_t XTCamera::set_fov(real_t fov)
{
	return m_p_fov = fov;
}

Ray XTCamera::get_primary_ray(int x, int y, XT_PROJECTION_T prjtype)
{
	Ray primary_ray;

	switch(prjtype)
	{
		case XT_PROJECTION_PERSPECTIVE:
			break;
		case XT_PROJECTION_ORTHOGRAPHIC:
			break;
	}

	return primary_ray;
}
