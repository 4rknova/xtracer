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

class TriangleLight : public Light
{
	public:
		TriangleLight();

		void geometry(const Vector3f &a, const Vector3f &b, const Vector3f &c);
		const Vector3f a() const;
		const Vector3f b() const;
		const Vector3f c() const;

		bool is_area_light() const;
		Vector3f point_sample() const;
		Ray ray_sample() const;

	private:
		Vector3f m_a, m_b, m_c;
};

#endif /* XTRACER_LIGHT_HPP_INCLUDED */
