#include <math.h>
#include "sphere.h"
#include "intinfo.h"
#include "sample.h"
#include "quadratic.h"

namespace NMath {

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

#ifdef __cplusplus
}

Sphere::Sphere()
    : Geometry(GEOMETRY_SPHERE), radius(NMATH_SPHERE_DEFAULT_RADIUS)
{}

Sphere::Sphere(const Vector3f &org, scalar_t rad)
    : Geometry(GEOMETRY_SPHERE), origin(org), radius(rad > 0 ? rad : NMATH_SPHERE_DEFAULT_RADIUS)
{}

bool Sphere::intersection(const Ray &ray, IntInfo* i_info) const
{

#ifdef NMATH_USE_BBOX_INTERSECTION
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

		if (t > EPSILON && i_info)
		{
			i_info->t = t;
			i_info->point = ray.origin + ray.direction * t;
			i_info->normal = (i_info->point - origin) / radius * (t1*t2 > 0. ? 1. : -1);
			i_info->texcoord = Vector2f((asin(i_info->normal.x / (uv_scale.x != 0.0f ? uv_scale.x : 1.0f)) / PI + 0.5), 
								(asin(i_info->normal.y / (uv_scale.y != 0.0f ? uv_scale.y : 1.0f)) / PI + 0.5));
			i_info->geometry = this;

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

    i_info->t = t0;
	i_info->point = ray.origin + ray.direction * t0;
	i_info->normal = (i_info->point - origin) / radius;
	i_info->texcoord = Vector2f((asin(i_info->normal.x / (uv_scale.x != 0.0f ? uv_scale.x : 1.0f)) / PI + 0.5),
	    						(asin(i_info->normal.y / (uv_scale.y != 0.0f ? uv_scale.y : 1.0f)) / PI + 0.5));
	i_info->geometry = this;

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
    return Vector3f(Sample::sphere() * radius) + origin;
}

Ray Sphere::ray_sample() const
{
    Ray ray;

    Vector3f sphpoint = Sample::sphere();
    Vector3f normal = sphpoint.normalized();

    ray.origin    = sphpoint * radius + origin;
    ray.direction = normal;// Sample::hemisphere(normal, normal);

    return ray;
}

#endif	/* __cplusplus */

} /* namespace NMath */
