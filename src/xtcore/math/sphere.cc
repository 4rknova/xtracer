#include <cmath>
#include <nmath/precision.h>
#include <nmath/sample.h>
#include <nmath/quadratic.h>
#include "sphere.h"
#include "hitrecord.h"

namespace xtcore {
    namespace surface {

Sphere::Sphere()
    : radius(XTCORE_SPHERE_DEFAULT_RADIUS)
{}

Sphere::Sphere(const Vector3f &org, scalar_t rad)
    : origin(org)
    , radius(rad > 0 ? rad : XTCORE_SPHERE_DEFAULT_RADIUS)
{}

nmath::scalar_t Sphere::distance(nmath::Vector3f p) const
{
    return (p - origin).length() - radius;
}

bool Sphere::intersection(const Ray &ray, hit_record_t* i_hit_record) const
{

#ifdef XTCORE_USE_BBOX_INTERSECTION
	if(!aabb.intersection(ray))
	{
		return false;
	}
#endif

#if 1
	scalar_t b = 2 * dot(ray.origin - origin, ray.direction);
	scalar_t c = dot(origin, origin) + dot(ray.origin, ray.origin) + 2 * dot(-origin, ray.origin) - radius * radius;

	scalar_t discr = (b * b - 4 * c);

	if (discr > 0.0)
	{
		scalar_t sqrt_discr = sqrt(discr);
		scalar_t t1 = (-b + sqrt_discr) / 2.0;
		scalar_t t2 = (-b - sqrt_discr) / 2.0;
		scalar_t t = t1 < t2 ? t1 : t2;

		if (t > EPSILON && i_hit_record)
		{
			i_hit_record->t = t;
			i_hit_record->point = ray.origin + ray.direction * t;
			i_hit_record->normal = (i_hit_record->point - origin) / radius * (t1*t2 > 0. ? 1. : -1);
            {
                const nmath::scalar_t sx = (uv_scale.x != 0.0f) ? uv_scale.x : (nmath::scalar_t)1.0;
                const nmath::scalar_t sy = (uv_scale.y != 0.0f) ? uv_scale.y : (nmath::scalar_t)1.0;
                const nmath::scalar_t u = (std::atan2(i_hit_record->normal.z, i_hit_record->normal.x) / (nmath::scalar_t)(2.0 * nmath::PI) + (nmath::scalar_t)0.5) * sx;
                const nmath::scalar_t v = (std::asin(nmath::clamp(i_hit_record->normal.y, (nmath::scalar_t)-1.0, (nmath::scalar_t)1.0)) / nmath::PI + (nmath::scalar_t)0.5) * sy;
                i_hit_record->texcoord = Vector2f(u, v);
            }

            i_hit_record->incident_direction = ray.direction;

			return true;
		}
	}

	return false;
#else
    scalar_t t0, t1;

    Vector3f L = ray.origin - origin;
    scalar_t a = dot(ray.direction, ray.direction);
    scalar_t b = 2.f * dot(ray.direction, L);
    scalar_t c = dot(L, L) - (radius*radius);

    if (!solve_quadratic(a, b, c, t0, t1)) return false;

    if (t0 > t1) std::swap(t0,t1);

    if (t0 < 0.f) {
        t0 = t1;
        if (t0 < 0.f) return false;
    }

    i_hit_record->t = t0;
	i_hit_record->point = ray.origin + ray.direction * t0;
	i_hit_record->normal = (i_hit_record->point - origin) / radius;
    {
        const nmath::scalar_t sx = (uv_scale.x != 0.0f) ? uv_scale.x : (nmath::scalar_t)1.0;
        const nmath::scalar_t sy = (uv_scale.y != 0.0f) ? uv_scale.y : (nmath::scalar_t)1.0;
        const nmath::scalar_t u = (std::atan2(i_hit_record->normal.z, i_hit_record->normal.x) / (nmath::scalar_t)(2.0 * nmath::PI) + (nmath::scalar_t)0.5) * sx;
        const nmath::scalar_t v = (std::asin(nmath::clamp(i_hit_record->normal.y, (nmath::scalar_t)-1.0, (nmath::scalar_t)1.0)) / nmath::PI + (nmath::scalar_t)0.5) * sy;
        i_hit_record->texcoord = Vector2f(u, v);
    }
	i_hit_record->geometry = this;

    return true;
#endif
}

void Sphere::calc_aabb()
{
	aabb.max = origin + Vector3f(radius, radius, radius);
	aabb.min = origin - Vector3f(radius, radius, radius);
}

Vector3f Sphere::point_sample() const
{
    return Vector3f(nmath::sample::sphere() * radius) + origin;
}

Ray Sphere::ray_sample() const
{
    Ray ray;

    Vector3f sphpoint = nmath::sample::sphere();
    Vector3f normal = sphpoint.normalized();

    ray.origin    = sphpoint * radius + origin;
    ray.direction = normal;// Sample::hemisphere(normal, normal);

    return ray;
}

Vector3f Sphere::emitter_position() const
{
    return origin;
}

    } /* namespace surface */
} /* namespace xtcore */
