/*

    This file is part of xtracer.

    camera.cpp
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

#include "camera.hpp"

#include "nmath/precision.h"
#include "nmath/matrix.h"


Camera::Camera()
	: position(Vector3(0,0,0)), target(Vector3(0,0,1)), up(Vector3(0,1,0)), fov(XT_CAM_DEFAULT_FOV)
{}

Camera::Camera(Vector3 &pos, Vector3 &trg, Vector3 &upv, scalar_t fovx, scalar_t aprt, scalar_t flen, scalar_t shut)
	:
	position(pos), 
	target(trg), 
	up(upv.normalized()), 
	fov(fovx),
	aperture(aprt),
	flength(flen),
	shutter(shut)
{}
#include <iostream>
Ray Camera::get_primary_ray(float x, float y, float width, float height)
{
	// Primary ray
	Ray pray;
	
	// Take the aspect ratio into consideration.
	scalar_t ratio = (scalar_t)width / (scalar_t)height;

	// Set the primary ray's origin at the camera's position.
	pray.origin = position;

	// Construct the ray's intersection point on the projection plane.
	pray.direction.x = (2.0 * (scalar_t)x / (scalar_t)width) - 1.0;
	pray.direction.y = ((2.0 * (scalar_t)y / (scalar_t)height) - 1.0) / ratio;

	pray.direction.z = 1 / tan(fov / 2.0);

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
	
	// Calculate the camera direction vector and normalize it.
	Vector3 camdir = target - position;
	camdir.normalize();

	Vector3 rx,ry,rz;

	rz = camdir;
	rx = cross(up, rz);
	rx.normalize();
	ry = cross(rx, rz);
	ry.normalize();

	Matrix4x4 tmat(	rx.x, ry.x, rz.x, 0, 
					rx.y, ry.y, rz.y, 0,
					rx.z, ry.z, rz.z, 0,
					0, 0, 0, 1);

	// Transform the direction vector
	pray.direction.transform(tmat);
	pray.direction.normalize();

	return pray;
}

#include <nmath/prng.h>
Ray Camera::get_primary_ray_dof(float x, float y, float width, float height, float dofx, float dofy)
{

//	return get_primary_ray(x, y, width, height);

	// Primary ray
	Ray pray, fray;
	
	// Take the aspect ratio into consideration.
	scalar_t ratio = (scalar_t)width / (scalar_t)height;

	// Set the primary ray's origin at the camera's position.
	pray.origin = position;

	// Calculate the ray's intersection point on the projection plane.
	pray.direction.x = (2.0 * (scalar_t)x / (scalar_t)width) - 1.0;
	pray.direction.y = -(1.0 - (2.0 * (scalar_t)y / (scalar_t)height)) / ratio;
	pray.direction.z = 1.0 / tan(fov / 2.0);

	// Calculate the deviated ray direction
	fray.origin = pray.direction;
	fray.origin.x += (prng_c(dofx - aperture/2, dofx + aperture/2) / 100) * aperture;
	fray.origin.y += (prng_c(dofy - aperture/2, dofy + aperture/2) / 100) * aperture;

	// Find the intersection point on the focal plane
	Vector3 fpip = pray.direction + flength * pray.direction.normalized();
	fray.direction = fpip - fray.origin;

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
	
	// Calculate the camera direction vector and normalize it.
	Vector3 camdir = target - position;
	camdir.normalize();

	Vector3 rx,ry,rz;

	rz = camdir;
	rx = cross(up, rz);
	rx.normalize();
	ry = cross(rx, rz);
	ry.normalize();

	Matrix4x4 tmat(	rx.x, ry.x, rz.x, 0, 
					rx.y, ry.y, rz.y, 0,
					rx.z, ry.z, rz.z, 0,
					0, 0, 0, 1);

	// Transform the direction vector
	fray.direction.transform(tmat);
	fray.direction.normalize();

	// Transform the origin of the ray
	fray.origin.transform(tmat);
	fray.origin += position;

	return fray;
}
