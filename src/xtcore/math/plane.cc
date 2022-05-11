#include <iostream>
#include <cmath>
#include "hitrecord.h"
#include "plane.h"

namespace xtcore {
    namespace surface {

Plane::Plane()
	: offset(XTCORE_PLANE_DEFAULT_DISTANCE)
{}

nmath::scalar_t Plane::distance(nmath::Vector3f p) const
{
     return dot(p, normal) + offset;
}

// algebraic solution
bool Plane::intersection(const Ray &ray, hit_record_t* i_hit_record) const
{
	// check if the ray is travelling parallel to the plane.
	// if the ray is in the plane then we ignore it.
	double n_dot_dir = dot(normal, ray.direction);

	if (fabs(n_dot_dir) < EPSILON) return false;

	Vector3f v = Vector3f(nmath_abs(normal.x), nmath_abs(normal.y), nmath_abs(normal.z)) * offset;

	Vector3f vorigin = v - ray.origin;

	double n_dot_vo = dot(vorigin, normal);

	double t = n_dot_vo / n_dot_dir;

	if (t < EPSILON) return false;

	if (i_hit_record)
	{
		i_hit_record->t = t;
		i_hit_record->point = ray.origin + ray.direction * t;
		i_hit_record->normal = normal;
        i_hit_record->incident_direction = ray.direction;

		// Texture coordinates.
		Vector3f n = normal.normalized();
		Vector3f uvec = Vector3f(n.y, n.z, -n.x);
		Vector3f vvec = cross(uvec, n);
		scalar_t tu = dot(uvec, (v+i_hit_record->point)) * uv_scale.x;
		scalar_t tv = dot(vvec, (v+i_hit_record->point)) * uv_scale.y;
		if (tu >  1.f) tu -= (float)(int)tu;
		if (tv >  1.f) tv -= (float)(int)tv;
		if (tu < -1.f) tu -= (float)(int)tu;
		if (tv < -1.f) tv -= (float)(int)tv;
		if (tu <  0.f) tu = 1.f + tu;
		if (tv <  0.f) tv = 1.f + tv;

		i_hit_record->texcoord = Vector2f(tu, tv);
	}

	return true;
}

void Plane::calc_aabb()
{
	// The plane is infinite so the bounding box is infinite as well
	aabb.max = Vector3f(INFINITY, INFINITY, INFINITY);
	aabb.min = -aabb.max;
}

Vector3f Plane::point_sample() const
{
    return Vector3f(0, 0, 0);
}

Ray Plane::ray_sample() const
{
    Ray ray;

    return ray;
}

    } /* namespace surface */
} /* namespace xtcore */
