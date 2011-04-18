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

#include <nmath/defs.h>
#include <nmath/precision.h>
#include <nmath/vector.h>
#include <nmath/ray.h>

#define XT_CAM_DEFAULT_FOV M_PI/4

/* Pinhole camera */
class Camera
{
	public:
		Camera();
		Camera(Vector3 &pos, Vector3 &trg, Vector3 &upv, real_t fovx=XT_CAM_DEFAULT_FOV);

		Ray get_primary_ray(unsigned int x, unsigned int y, unsigned int width, unsigned int height);

		Vector3 position;
		Vector3 target;
		Vector3 up;
		real_t fov;
};

#endif /* XTRACER_CAMERA_HPP_INCLUDED */
