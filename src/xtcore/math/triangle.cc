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
    v[0] = nmath::Vector3f(0.0f, 0.0f, 0.0f);
    v[1] = nmath::Vector3f(0.0f, 0.0f, 0.0f);
    v[2] = nmath::Vector3f(0.0f, 0.0f, 0.0f);
    n[0] = nmath::Vector3f(0.0f, 0.0f, 0.0f);
    n[1] = nmath::Vector3f(0.0f, 0.0f, 0.0f);
    n[2] = nmath::Vector3f(0.0f, 0.0f, 0.0f);
    tc[0] = nmath::Vector2f(0.0f, 0.0f);
    tc[1] = nmath::Vector2f(1.0f, 0.0f);
    tc[2] = nmath::Vector2f(0.0f, 1.0f);
    edge1 = nmath::Vector3f(0.0f, 0.0f, 0.0f);
    edge2 = nmath::Vector3f(0.0f, 0.0f, 0.0f);
    face_normal = nmath::Vector3f(0.0f, 0.0f, 0.0f);
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
    // Moller-Trumbore intersection (two-sided).
    const Vector3f local_edge1 = edge1.length_squared() > (scalar_t)0.0 ? edge1 : (v[1] - v[0]);
    const Vector3f local_edge2 = edge2.length_squared() > (scalar_t)0.0 ? edge2 : (v[2] - v[0]);
    const Vector3f pvec = cross(ray.direction, local_edge2);
    const scalar_t det = dot(local_edge1, pvec);

    if (nmath_abs(det) < EPSILON) return false;

    const scalar_t inv_det = 1.0f / det;
    const Vector3f tvec = ray.origin - v[0];
    const scalar_t u = dot(tvec, pvec) * inv_det;
    if (u < 0.0f || u > 1.0f) return false;

    const Vector3f qvec = cross(tvec, local_edge1);
    const scalar_t vv = dot(ray.direction, qvec) * inv_det;
    if (vv < 0.0f || (u + vv) > 1.0f) return false;

    const scalar_t t = dot(local_edge2, qvec) * inv_det;
    if (t < EPSILON) return false;

    if (i_hit_record) {
        const scalar_t w = 1.0f - u - vv;
        const Vector3f pos = ray.origin + ray.direction * t;
        Vector3f local_face_normal = face_normal;
        if (!local_face_normal.length_squared()) {
            local_face_normal = cross(local_edge1, local_edge2);
            if (local_face_normal.length()) local_face_normal = local_face_normal.normalized();
        }

        i_hit_record->t = t;
        i_hit_record->point = pos;
        i_hit_record->texcoord = tc[0] * w + tc[1] * u + tc[2] * vv;

        Vector3f pn = n[0] * w + n[1] * u + n[2] * vv;
        i_hit_record->normal = pn.length() ? pn.normalized() : local_face_normal;
        i_hit_record->incident_direction = ray.direction;
    }

    return true;
}

void Triangle::calc_aabb()
{
	aabb.max = Vector3f(-INFINITY, -INFINITY, -INFINITY);
	aabb.min = Vector3f( INFINITY,  INFINITY,  INFINITY);
    edge1 = v[1] - v[0];
    edge2 = v[2] - v[0];
    face_normal = cross(edge1, edge2);
    if (face_normal.length()) face_normal = face_normal.normalized();

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

    const scalar_t u = nmath::prng_c(0, 1);
    const scalar_t w = nmath::prng_c(0, 1);
    const scalar_t su = nmath_sqrt(std::max((scalar_t)0.0, u));

    b0 = (scalar_t)1.0 - su;
    b1 = su * ((scalar_t)1.0 - w);
    b2 = su * w;

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

Vector3f Triangle::emitter_position() const
{
    return (v[0] + v[1] + v[2]) / 3.0f;
}

    } /* namespace surface */
} /* namespace xtcore */
