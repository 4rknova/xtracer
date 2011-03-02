/*

    This file is part of xtracer.

    camera.hpp
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

#ifndef XTRACER_CAMERA_HPP_INCLUDED
#define XTRACER_CAMERA_HPP_INCLUDED

#include <nmath/vector.h>
#include <nmath/ray.h>
#include <nmath/precision.h>

enum XT_PROJECTION_T
{
	XT_PROJECTION_PERSPECTIVE,
	XT_PROJECTION_ORTHOGRAPHIC
};

#define XT_DEFAULT_PROJECTION XT_PROJECTION_PERSPECTIVE

class XTCamera
{
	public:
		XTCamera();
		XTCamera(Vector3 &position, Vector3 &angle, real_t fovx, XT_PROJECTION_T prjt = XT_DEFAULT_PROJECTION);

		Vector3 get_position();
		Vector3 get_angle();
		real_t get_fov();
		XT_PROJECTION_T get_prjtype();

		Vector3 set_position(Vector3 &position);
		Vector3 set_angle(Vector3 &angle);
		real_t set_fov(real_t fov);
		XT_PROJECTION_T get_prjtype(XT_PROJECTION_T prjt);

		Ray get_primary_ray(unsigned int x, unsigned int y, unsigned int width, unsigned int height);

	private:
		Vector3 m_p_position;
		Vector3 m_p_angle;
		real_t m_p_fov_x; 					/* Field of view x */
		XT_PROJECTION_T m_p_prjt;			/* Projection type */
};

#endif /* XTRACER_CAMERA_HPP_INCLUDED */
