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

#include <nmath/precision.h>
#include <nmath/matrix.h>


Camera::Camera()
	: m_p_position(Vector3(0,0,0)), m_p_target(Vector3(0,0,1)), m_p_up(Vector3(0,1,0)), m_p_fov(XT_CAM_DEFAULT_FOV)
{}

Camera::Camera(Vector3 &position, Vector3 &target, Vector3 &up, real_t fovx)
:  m_p_position(position), m_p_target(target), m_p_up(up.normalized()), m_p_fov(fovx)
{}

Vector3 Camera::position()
{
	return m_p_position;
}

Vector3 Camera::target()
{
	return m_p_target;
}

Vector3 Camera::up()
{
	return m_p_up;
}

real_t Camera::fov()
{
	return m_p_fov;
}

Vector3 Camera::set_position(Vector3 &position)
{
	return m_p_position = position;
}

Vector3 Camera::set_target(Vector3 &target)
{
	return m_p_target = target;
}

Vector3 Camera::set_up(Vector3 &up)
{
	return m_p_up = up.normalized();
}

real_t Camera::set_fov(real_t fov)
{
	return m_p_fov = fov;
}

Ray Camera::get_primary_ray(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
	Ray pray;		/* Primary ray */

	/* Set the primary ray's origin at the camera's position. */
	pray.origin = m_p_position;

	/* Construct the ray's direction vector. */
	pray.direction.x = (2.0 * (real_t)x / (real_t)width) - 1.0;
	pray.direction.y = (1.0 - ((2.0 * (real_t)y) / (real_t)height)) * ((real_t)height / (real_t)width);
	pray.direction.z = 1 / tan(m_p_fov / 2.0);

	/* Transform the camera target vector to world coordinates and normalize it. */
	Vector3 camdir = m_p_target - m_p_position;
	camdir.normalize();

	/*
		Setting up the look-at matrix is easy when you consider that a matrix 
		is basically a rotated unit cube formed by three vectors (the 3x3 part) at a 
		particular position (the 1x3 part). 
		
		We already have one of the three vectors: 
			- 	The z-axis of the matrix is simply the view direction.
			- 	The x-axis of the matrix is a bit tricky: if the camera is not tilted, 
				then the x-axis of the matrix is perpendicular to the z-axis and 
				the vector (0, 1, 0). 
			-	The y-axis is perpendicular to the other two, so we simply calculate
				the cross product of the x-axis and the z-axis to obtain the y-axis.
				Note that the y-axis is calculated using the reversed z-axis. The 
				image will be upside down without this adjustment.
	*/

	Vector3 rx,ry,rz;

	rz = camdir;
	rx = cross(m_p_up, rz);
	ry = cross(x, -rz);

	Matrix4x4 tmat(	rx.x, ry.x, rz.x, 0, 
					rx.y, ry.y, rz.y, 0,
					rx.z, ry.z, rz.z, 0,
					0, 0, 0, 1);

	/* Transform the direction vector */
	pray.direction.transform(tmat);

	return pray;
}
