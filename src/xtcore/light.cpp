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

	return position() + (v / 2);
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
			v.y = -m_dimensions.y;
			normal = Vector3f(0, -1, 0);
			break;
		case 6:
			v.x = ra * m_dimensions.x;
			v.z = rb * m_dimensions.z;
			v.y = m_dimensions.y;
			normal = Vector3f(0, 1, 0);
			break;
	}

	ray.origin = position() + (v / 2);
	ray.direction = NMath::Sample::hemisphere(normal, normal);

	return ray;
}

TriangleLight::TriangleLight()
{}

bool TriangleLight::is_area_light() const
{
	return true;
}

void TriangleLight::geometry(const Vector3f &a, const Vector3f &b, const Vector3f &c)
{
	m_a = a;
	m_b = b;
	m_c = c;
}

const Vector3f TriangleLight::a() const
{
	return m_a;
}

const Vector3f TriangleLight::b() const
{
	return m_b;
}

const Vector3f TriangleLight::c() const
{
	return m_c;
}

Vector3f TriangleLight::point_sample() const
{
	scalar_t b0, b1, b2;

	b0 = NMath::prng_c(0, 1);
	b1 = NMath::prng_c(0, 1);
	b2 = NMath::prng_c(0, 1);

	scalar_t bt = b0 + b1 + b2;

	b0 /= bt;
	b1 /= bt;
	b2 /= bt;

	return Vector3f(b0 * m_a + b1 * m_b + b2 * m_c) + position();
}

Ray TriangleLight::ray_sample() const
{
	Ray ray;

	// Origin.
	ray.origin = point_sample();

	// Direction.
	Vector3f normal = cross(m_b - m_a, m_c - m_a).normalized();
	ray.direction = NMath::Sample::hemisphere(normal, normal);

	return ray;
}
