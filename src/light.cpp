/*

    This file is part of xtracer.

    light.cpp
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

#include <nmath/prng.h>
#include <nmath/sample.h>
#include "light.hpp"

using NMath::vec3_t;

/* Light interface */
Light::Light(const Vector3f &pos, const ColorRGBf &ints)
	: m_position(pos), 
	  m_intensity(ints)
{}

Light::Light()
	: m_position(Vector3f(0, 0, 0)), 
	  m_intensity(ColorRGBf(1, 1, 1))
{}

Light::~Light()
{}

const Vector3f &Light::position() const
{
	return m_position;
}

const ColorRGBf &Light::intensity() const
{
	return m_intensity;
}

void Light::position(const Vector3f &position)
{
	m_position = position;
}

void Light::intensity(const ColorRGBf &intensity)
{
	m_intensity = intensity;
}

/* Point light */
bool PointLight::is_area_light() const
{
	return false;
}

Vector3f PointLight::point_sample() const
{
	return position();
}

/* Sphere light */
SphereLight::SphereLight()
	: m_radius(0.1f)
{}

scalar_t SphereLight::radius() const
{
	return m_radius;
}

void SphereLight::radius(scalar_t r)
{
	m_radius = r;
}

bool SphereLight::is_area_light() const
{
	return true;
}

Vector3f SphereLight::point_sample() const
{
	return Vector3f(NMath::Sample::sphere() * m_radius) + position();
}

/* Box light */
BoxLight::BoxLight()
	: m_dimensions(Vector3f(0.5, 0.1, 0.5))
{}

const Vector3f &BoxLight::dimensions() const
{
	return m_dimensions;
}

void BoxLight::dimensions(const Vector3f &dimensions)
{
	m_dimensions = dimensions;
}

bool BoxLight::is_area_light() const
{
	return true;
}

Vector3f BoxLight::point_sample() const
{
	Vector3f v;
    v.x = NMath::prng_c(0, m_dimensions.x) - 0.5f * m_dimensions.x;
	v.y = NMath::prng_c(0, m_dimensions.y) - 0.5f * m_dimensions.y;
	v.z = NMath::prng_c(0, m_dimensions.z) - 0.5f * m_dimensions.z;

	return position() + v;
}
