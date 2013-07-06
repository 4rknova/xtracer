/*

    This file is part of xtracer.

    camera.hpp
    Camera class

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

#ifndef XTRACER_CAMERA_HPP_INCLUDED
#define XTRACER_CAMERA_HPP_INCLUDED

#include "defs.h"
#include "precision.h"
#include "vector.h"
#include "ray.h"

using NMath::scalar_t;
using NMath::Vector3f;
using NMath::Ray;

#define XT_CAM_DEFAULT_FOV M_PI / 4

/* Pinhole camera */
class Camera
{
	public:
		Camera();
		Camera(Vector3f &pos, Vector3f &trg, Vector3f &upv, scalar_t fovx=XT_CAM_DEFAULT_FOV, scalar_t aprt=1.0, scalar_t flen=0.0, scalar_t shut=0.01);

		Ray get_primary_ray(float x, float y, float width, float height);
		Ray get_primary_ray_dof(float x, float y, float width, float height);

		Vector3f position;
		Vector3f target;
		Vector3f up;
		scalar_t fov;

		scalar_t aperture;
		scalar_t flength;
};

#endif /* XTRACER_CAMERA_HPP_INCLUDED */
