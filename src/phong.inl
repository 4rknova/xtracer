/*

    This file is part of xtracer.

    phong.inl
   	Phong

    Copyright (C) 2008, 2010
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

#ifndef XTRACER_PHONG_INL_INCLUDED
#define XTRACER_PHONG_INL_INCLUDED

#include <math.h>

#ifndef XTRACER_PHONG_HPP_INCLUDED
    #error "phong.hpp must be included before phong.inl"
#endif /* XTRACER_PHONG_HPP_INCLUDED */

inline Vector3 phong(const Vector3 &campos, const Vector3 &lightpos, const IntInfo *info, const Vector3 &light, real_t ks, real_t kd, real_t specexp, Vector3 &diffuse, Vector3 &specular)
{
	// calculate the light direction vector
	Vector3 lightdir = lightpos - info->point;
	lightdir.normalize();

	// calculate the normal - light dot product
	real_t d = dot(lightdir, info->normal);

	if (d < 0.0)
		d = 0;

	Vector3 ray = campos - info->point;
	ray.normalize();

	Vector3 r = lightdir.reflected(info->normal);
	r.normalize();

	real_t rmv = dot(r, ray);

	if (rmv < 0.0)
		rmv = 0;

	return ((kd * d * diffuse) + (ks * pow(rmv, specexp) * specular)) * light;
}

inline Vector3 blinn_phong(const Vector3 &campos, const Vector3 &lightpos, const IntInfo *info, const Vector3 &light, real_t ks, real_t kd, real_t specexp, Vector3 &diffuse, Vector3 &specular)
{
	// calculate the light direction vector
	Vector3 lightdir = lightpos - info->point;
	lightdir.normalize();

	// calculate the normal - light dot product
	real_t d = dot(lightdir, info->normal);

	if (d < 0.0)
		d = 0;

	Vector3 ray = campos - info->point;
	ray.normalize();

	Vector3 r = lightdir + ray;
	r.normalize();

	real_t rmv = dot(r, info->normal);

	if (rmv < 0.0)
		rmv = 0;

	return ((kd * d * diffuse) + (ks * pow(rmv, specexp) * specular)) * light;
	
}

#endif /* XTRACER_PHONG_INL_INCLUDED */
