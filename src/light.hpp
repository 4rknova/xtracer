/*

    This file is part of xtracer.

    light.hpp
    Light class

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

#ifndef XTRACER_LIGHT_HPP_INCLUDED
#define XTRACER_LIGHT_HPP_INCLUDED

#include <nmath/precision.h>
#include <nmath/vector.h>
#include <nmath/ray.h>
#include <nimg/color.hpp>

using NMath::scalar_t;
using NMath::Vector3f;
using NMath::Ray;
using NImg::ColorRGBf;

/* Light interface */
class Light 
{
	public:
		Light(const Vector3f &pos, const ColorRGBf &ints);
		Light();
		virtual ~Light();

		virtual bool is_area_light() const = 0;

		virtual Vector3f point_sample() const = 0;
		virtual Ray ray_sample() const = 0;

		const Vector3f &position() const;
		const ColorRGBf &intensity() const;
		void position(const Vector3f &position);
		void intensity(const ColorRGBf &intensity); 

	private:
		Vector3f m_position;
		ColorRGBf m_intensity;
};

class PointLight : public Light
{
	public:
		bool is_area_light() const;
		Vector3f point_sample() const;
		Ray ray_sample() const;
};

class SphereLight : public Light
{
	public:
		SphereLight();

		scalar_t radius() const;
		void radius(scalar_t r);
		
		bool is_area_light() const;
		Vector3f point_sample() const;
		Ray ray_sample() const;

	private:
		scalar_t m_radius;
};

class BoxLight : public Light
{
	public:
		BoxLight();

		const Vector3f &dimensions() const;
		void dimensions(const Vector3f &dimensions);

		bool is_area_light() const;
		Vector3f point_sample() const;
		Ray ray_sample() const;

	private:
		Vector3f m_dimensions;
};

#endif /* XTRACER_LIGHT_HPP_INCLUDED */
