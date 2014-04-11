/*

    This file is part of xtracer.

    phong.inl
   	Phong

    Copyright (C) 2008, 2010
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

#ifndef XTRACER_PHONG_INL_INCLUDED
#define XTRACER_PHONG_INL_INCLUDED

#include <iostream>
#include <math.h>

#ifndef XTRACER_PHONG_HPP_INCLUDED
    #error "phong.hpp must be included before phong.inl"
#endif /* XTRACER_PHONG_HPP_INCLUDED */

inline ColorRGBf phong(const Vector3f &campos, const Vector3f &lightpos, const IntInfo *info,
					 const ColorRGBf &light, scalar_t ks, scalar_t kd, scalar_t specexp,
					 const ColorRGBf &diffuse, const ColorRGBf &specular)
{
	// calculate the light direction vector
	Vector3f lightdir = lightpos - info->point;
	lightdir.normalize();

	// calculate the normal - light dot product
	scalar_t d = dot(lightdir, info->normal);

	if (d < 0.0)
		d = 0;

	Vector3f ray = campos - info->point;
	ray.normalize();

	Vector3f r = lightdir.reflected(info->normal);
	r.normalize();

	scalar_t rmv = dot(r, ray);

	if (rmv < 0.0)
		rmv = 0;

	return ((kd * d * diffuse) + (ks * pow((long double)rmv, (long double)specexp) * specular)) * light;
}

inline ColorRGBf blinn_phong(const Vector3f &campos, const Vector3f &lightpos, const IntInfo *info,
						   const ColorRGBf &light, scalar_t ks, scalar_t kd, scalar_t specexp,
						   const ColorRGBf &diffuse, const ColorRGBf &specular)
{
	// calculate the light direction vector
	Vector3f lightdir = lightpos - info->point;
	lightdir.normalize();

	// calculate the normal - light dot product
	scalar_t d = dot(lightdir, info->normal);

	if (d < 0.0)
		d = 0;

	Vector3f ray = campos - info->point;
	ray.normalize();

	Vector3f r = lightdir + ray;
	r.normalize();

	scalar_t rmv = dot(r, info->normal);

	if (rmv < 0.0)
		rmv = 0;

	return ((kd * d * diffuse) + (ks * pow(rmv, specexp) * specular)) * light;
}

#endif /* XTRACER_PHONG_INL_INCLUDED */
