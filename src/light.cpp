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

Ray PointLight::ray_sample() const
{
	Ray ray;
	ray.origin = position();
	ray.direction = NMath::Sample::sphere();
	return ray;
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

Ray SphereLight::ray_sample() const
{
	Ray ray;

	Vector3f sphpoint = Vector3f(NMath::Sample::sphere() * m_radius);
	Vector3f normal = sphpoint.normalized();

	ray.origin = sphpoint + position();
	ray.direction = NMath::Sample::hemisphere(normal, normal);

	return ray;
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

	unsigned int side = (unsigned int)NMath::prng_c(1, 7);
	scalar_t ra = NMath::prng_c(0, 1) - 0.5;
	scalar_t rb = NMath::prng_c(0, 1) - 0.5;

	switch (side) {
		case 1:
			v.x = ra * m_dimensions.x;
			v.y = rb * m_dimensions.y;
			v.z = -m_dimensions.z;
			break;
		case 2:
			v.z = ra * m_dimensions.z;
			v.y = rb * m_dimensions.y;
			v.x = m_dimensions.x;
			break;
		case 3:
			v.x = ra * m_dimensions.x;
			v.y = rb * m_dimensions.y;
			v.z = m_dimensions.z;
			break;
		case 4:
			v.z = ra * m_dimensions.z;
			v.y = rb * m_dimensions.y;
			v.x = -m_dimensions.x;
			break;
		case 5:
			v.x = ra * m_dimensions.x;
			v.z = rb * m_dimensions.z;
			v.y = m_dimensions.y;
			break;
		case 6:
			v.x = ra * m_dimensions.x;
			v.z = rb * m_dimensions.z;
			v.y = -m_dimensions.y;
			break;
	}

	return position() + v;
}

Ray BoxLight::ray_sample() const {
	Vector3f v, normal;
	Ray ray;
	
	unsigned int side = (unsigned int)NMath::prng_c(1, 7);
	scalar_t ra = NMath::prng_c(0, 1) - 0.5;
	scalar_t rb = NMath::prng_c(0, 1) - 0.5;

	switch (side) {
		case 1:
			v.x = ra * m_dimensions.x;
			v.y = rb * m_dimensions.y;
			v.z = -m_dimensions.z;
			normal = Vector3f(0, 0, -1);
			break;
		case 2:
			v.z = ra * m_dimensions.z;
			v.y = rb * m_dimensions.y;
			v.x = m_dimensions.x;
			normal = Vector3f(1, 0, 0);
			break;
		case 3:
			v.x = ra * m_dimensions.x;
			v.y = rb * m_dimensions.y;
			v.z = m_dimensions.z;
			normal = Vector3f(0, 0, 1);
			break;
		case 4:
			v.z = ra * m_dimensions.z;
			v.y = rb * m_dimensions.y;
			v.x = -m_dimensions.x;
			normal = Vector3f(-1, 0, 0);
			break;
		case 5:
			v.x = ra * m_dimensions.x;
			v.z = rb * m_dimensions.z;
			v.y = m_dimensions.y;
			normal = Vector3f(0, 1, 0);
			break;
		case 6:
			v.x = ra * m_dimensions.x;
			v.z = rb * m_dimensions.z;
			normal = Vector3f(0, -1, 0);
			break;
	}

	ray.origin = position() + v;
	ray.direction = NMath::Sample::hemisphere(normal, normal);
}
