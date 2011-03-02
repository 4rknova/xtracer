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
#include <nmath/precision.h>
#include <nmath/ray.h>
#include <nmath/vector.h>
#include <nmath/matrix.h>

#define XTRACER_DEFAULT_FOV M_PI/4

XTCamera::XTCamera()
	: m_p_position(Vector3(0,0,0)), m_p_angle(Vector3(0,0,0)), m_p_fov_x(XTRACER_DEFAULT_FOV), m_p_prjt(XT_DEFAULT_PROJECTION)
{}

XTCamera::XTCamera(Vector3 &position, Vector3 &angle, real_t fovx, XT_PROJECTION_T prjt)
:  m_p_position(position), m_p_angle(angle), m_p_fov_x(fovx), m_p_prjt(prjt)
{}

Vector3 XTCamera::get_position()
{
	return m_p_position;
}

Vector3 XTCamera::get_angle()
{
	return m_p_angle;
}

real_t XTCamera::get_fov()
{
	return m_p_fov_x;
}

XT_PROJECTION_T XTCamera::get_prjtype()
{
	return m_p_prjt;
}

Vector3 XTCamera::set_position(Vector3 &position)
{
	return m_p_position = position;
}

Vector3 XTCamera::set_angle(Vector3 &angle)
{
	return m_p_angle = angle;
}

real_t XTCamera::set_fov(real_t fov)
{
	return m_p_fov_x = fov;
}

XT_PROJECTION_T XTCamera::get_prjtype(XT_PROJECTION_T prjt)
{
	return m_p_prjt = prjt;
}

Ray XTCamera::get_primary_ray(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
	Ray primary_ray;

	switch(m_p_prjt)
	{
		case XT_PROJECTION_PERSPECTIVE:
			{
				real_t theta = m_p_fov_x / 2;
				real_t cx = ((2 * x) / width) - 1;
				real_t cy = (((2 * y) / height) - 1) * (height / width);
				real_t cz = 1 / tan(theta);
				primary_ray.direction = Vector3(cx, cy, cz);
			}
			break;
		case XT_PROJECTION_ORTHOGRAPHIC:
			{
				real_t cx = ((2 * x) / width) - 1;
				real_t cy = (((2 * y) / height) - 1) * (height / width);
				
				primary_ray.origin = Vector3(cx, cy ,0);
				primary_ray.direction = Vector3(cx, cy, -1);
			}
			break;
	}

	Matrix4x4 tmat;

	/* Prepare the transformation matrix */
	tmat.rotate(Vector3(m_p_angle.x, m_p_angle.y, m_p_angle.z));
	tmat.translate(Vector3(m_p_position.x, m_p_position.y, m_p_position.z));

	/* Transform the ray */
	primary_ray.origin = Vector3(tmat * Vector4(primary_ray.origin));
	primary_ray.direction = Vector3(tmat * Vector4(primary_ray.direction));

	return primary_ray;
}

