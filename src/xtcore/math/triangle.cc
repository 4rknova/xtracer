#include <nmath/precision.h>
#include <nmath/vector.h>
#include <nmath/sample.h>

#include "hitrecord.h"
#include "prng.h"
#include "triangle.h"

namespace xtcore {
    namespace surface {

Triangle::Triangle()
{
    tc[0] = nmath::Vector2f(0.0f, 0.0f);
    tc[1] = nmath::Vector2f(1.0f, 1.0f);
    tc[1] = nmath::Vector2f(1.0f, 0.0f);
}

nmath::scalar_t Triangle::distance(nmath::Vector3f p) const
{
    nmath::Vector3f ba = v[1] - v[0]; nmath::Vector3f pa = p - v[0];
    nmath::Vector3f cb = v[2] - v[1]; nmath::Vector3f pb = p - v[1];
    nmath::Vector3f ac = v[0] - v[2]; nmath::Vector3f pc = p - v[2];
    nmath::Vector3f nor = cross(ba, ac);

    bool k = (nmath::sign(dot(cross(ba, nor), pa))
              + nmath::sign(dot(cross(cb, nor), pb))
              + nmath::sign(dot(cross(ac, nor), pc))) < 2.0;

    nmath::Vector3f v0 = ba * nmath::clamp(dot(ba, pa) / dot(ba, ba), 0.0, 1.0) - pa;
    nmath::Vector3f v1 = cb * nmath::clamp(dot(cb, pb) / dot(cb, cb), 0.0, 1.0) - pb;
    nmath::Vector3f v2 = ac * nmath::clamp(dot(ac, pc) / dot(ac, ac), 0.0, 1.0) - pc;

    nmath::scalar_t m = nmath::min3(dot(v0, v0), dot(v1, v1), dot(v2, v2));

    return nmath_sqrt(k ? m : dot(nor,pa)*dot(nor,pa)/dot(nor,nor));
}

/* To do: replace with  Möller–Trumbore ray-triangle intersection algorithm
 * from: https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
 *
 * bool RayIntersectsTriangle(Vector3D rayOrigin,
                           Vector3D rayVector,
                           Triangle* inTriangle,
                           Vector3D& outIntersectionPoint)
{
    const float EPSILON = 0.0000001;
    Vector3D vertex0 = inTriangle->vertex0;
    Vector3D vertex1 = inTriangle->vertex1;
    Vector3D vertex2 = inTriangle->vertex2;
    Vector3D edge1, edge2, h, s, q;
    float a,f,u,v;
    edge1 = vertex1 - vertex0;
    edge2 = vertex2 - vertex0;
    h = rayVector.crossProduct(edge2);
    a = edge1.dotProduct(h);
    if (a > -EPSILON && a < EPSILON)
        return false;    // This ray is parallel to this triangle.
    f = 1.0/a;
    s = rayOrigin - vertex0;
    u = f * s.dotProduct(h);
    if (u < 0.0 || u > 1.0)
        return false;
    q = s.crossProduct(edge1);
    v = f * rayVector.dotProduct(q);
    if (v < 0.0 || u + v > 1.0)
        return false;
    // At this stage we can compute t to find out where the intersection point is on the line.
    float t = f * edge2.dotProduct(q);
    if (t > EPSILON) // ray intersection
    {
        outIntersectionPoint = rayOrigin + rayVector * t;
        return true;
    }
    else // This means that there is a line intersection but not a ray intersection.
        return false;
}
 
*/


bool Triangle::intersection(const Ray &ray, hit_record_t* i_hit_record) const
{
	Vector3f normal = calc_normal();
	scalar_t n_dot_dir = dot(normal, ray.direction);

	if (fabs(n_dot_dir) < EPSILON) return false; // parallel to the plane

	// translation of v[0] to axis origin
	Vector3f vo_vec = ray.origin - v[0];

	// calc intersection distance
	scalar_t t = -dot(normal, vo_vec) / n_dot_dir;

	if (t < EPSILON) return false; // plane in the opposite subspace

	// intersection point ( on the plane ).
	Vector3f pos = ray.origin + ray.direction * t;

	// calculate barycentric
	Vector3f bc = calc_barycentric(pos);
	scalar_t bc_sum = bc.x + bc.y + bc.z;

	// check for triangle boundaries
	if (bc_sum < 1.0 - EPSILON || bc_sum > 1.0 + EPSILON) return false;

	if (i_hit_record)
	{
		i_hit_record->t = t;
		i_hit_record->point = pos;

		// Texcoords
		Vector2f texcoord = tc[0] * bc.x + tc[1] * bc.y + tc[2] * bc.z;
		i_hit_record->texcoord = texcoord;

		// Normal
		Vector3f pn = n[0] * bc.x + n[1] * bc.y + n[2] * bc.z;
		i_hit_record->normal = pn.length() ? pn : normal;
        i_hit_record->incident_direction = ray.direction;
	}

	return true;
}

void Triangle::calc_aabb()
{
	aabb.max = Vector3f(-INFINITY, -INFINITY, -INFINITY);
	aabb.min = Vector3f( INFINITY,  INFINITY,  INFINITY);

	for(unsigned int i=0; i<3; i++)
	{
		Vector3f pos = v[i];
		if(pos.x < aabb.min.x) aabb.min.x = pos.x;
		if(pos.y < aabb.min.y) aabb.min.y = pos.y;
		if(pos.z < aabb.min.z) aabb.min.z = pos.z;
		if(pos.x > aabb.max.x) aabb.max.x = pos.x;
		if(pos.y > aabb.max.y) aabb.max.y = pos.y;
		if(pos.z > aabb.max.z) aabb.max.z = pos.z;
	}
}

Vector3f Triangle::calc_normal() const
{
	Vector3f v1 = v[1] - v[0];
	Vector3f v2 = v[2] - v[0];
	return (cross(v1, v2)).normalized();
}

Vector3f Triangle::calc_barycentric(const Vector3f &p) const
{
	Vector3f bc(0.0f, 0.0f, 0.0f);

	Vector3f v1 = v[1] - v[0];
	Vector3f v2 = v[2] - v[0];
	Vector3f xv1v2 = cross(v1, v2);

	Vector3f norm = xv1v2.normalized();

	scalar_t area = fabs(dot(xv1v2, norm)) * 0.5;

	if(area < EPSILON)
	{
		return bc;
	}

	Vector3f pv0 = v[0] - p;
	Vector3f pv1 = v[1] - p;
	Vector3f pv2 = v[2] - p;

	// calculate the area of each sub-triangle
	Vector3f x12 = cross(pv1, pv2);
	Vector3f x20 = cross(pv2, pv0);
	Vector3f x01 = cross(pv0, pv1);

	scalar_t a0 = fabs(dot(x12, norm)) * 0.5;
	scalar_t a1 = fabs(dot(x20, norm)) * 0.5;
	scalar_t a2 = fabs(dot(x01, norm)) * 0.5;

	bc.x = a0 / area;
	bc.y = a1 / area;
	bc.z = a2 / area;

	return bc;
}

Vector3f Triangle::point_sample() const
{
    scalar_t b0, b1, b2;

    b0 = nmath::prng_c(0, 1);
    b1 = nmath::prng_c(0, 1);
    b2 = nmath::prng_c(0, 1);

    scalar_t bt = b0 + b1 + b2;

    b0 /= bt;
    b1 /= bt;
    b2 /= bt;

    return Vector3f(b0 * v[0] + b1 * v[1] + b2 * v[2]);
}

Ray Triangle::ray_sample() const
{
    Ray ray;

    ray.origin = point_sample();

    Vector3f normal = calc_normal();
    ray.direction = nmath::sample::hemisphere(normal, normal);
    ray.origin += ray.direction * EPSILON;

    return ray;
}

    } /* namespace surface */
} /* namespace xtcore */
