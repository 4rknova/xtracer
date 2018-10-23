#include "aabb.h"

namespace xtcore {

BoundingBox2::BoundingBox2()
{}

BoundingBox2::BoundingBox2(const Vector2f& a, const Vector2f& b)
{
    min=Vector2f((a.x<=b.x)? a.x : b.x, (a.y<=b.y)? a.y : b.y);
    max=Vector2f((a.x>=b.x)? a.x : b.x, (a.y>=b.y)? a.y : b.y);
}

BoundingBox3::BoundingBox3()
{}

BoundingBox3::BoundingBox3(const Vector3f& a, const Vector3f& b)
{
    min=Vector3f((a.x<=b.x)? a.x : b.x, (a.y<=b.y)? a.y : b.y, (a.z<=b.z)? a.z : b.z);
    max=Vector3f((a.x>=b.x)? a.x : b.x, (a.y>=b.y)? a.y : b.y, (a.z>=b.z)? a.z : b.z);
}

/*
    Amy Williams, Steve Barrus, R. Keith Morley, and Peter Shirley. 2005.
    An efficient and robust ray-box intersection algorithm. In ACM SIGGRAPH 2005
    Courses (SIGGRAPH '05), John Fujii (Ed.). ACM, New York, NY, USA, Article 9 .
    DOI: https://doi.org/10.1145/1198555.1198748

    https://dl.acm.org/citation.cfm?id=1198748
    http://people.csail.mit.edu/amy/papers/box-jgt.pdf
*/
bool BoundingBox3::intersection(const Ray &ray) const
{
	if (ray.origin > min && ray.origin < max)
		return true;

	Vector3f aabb[2] = {min, max};
	static const float t0 = 0.0;

    /* The sign and inverse quantities can
       be precalculated in the ray object
       during construction.
    */
	int xsign = (int)(ray.direction.x < 0.0);
	int ysign = (int)(ray.direction.y < 0.0);
	float invdirx = 1.0 / ray.direction.x;
	float invdiry = 1.0 / ray.direction.y;

	float tmin  = (aabb[    xsign].x - ray.origin.x) * invdirx;
	float tmax  = (aabb[1 - xsign].x - ray.origin.x) * invdirx;
	float tymin = (aabb[    ysign].y - ray.origin.y) * invdiry;
	float tymax = (aabb[1 - ysign].y - ray.origin.y) * invdiry;

	if ((tmin > tymax) || (tymin > tmax)) return false;
	if (tymin > tmin) tmin = tymin;
	if (tymax < tmax) tmax = tymax;

	int zsign = (int)(ray.direction.z < 0.0);
	float invdirz = 1.0 / ray.direction.z;

	float tzmin = (aabb[zsign].z - ray.origin.z) * invdirz;
	float tzmax = (aabb[1 - zsign].z - ray.origin.z) * invdirz;

	if ((tmin > tzmax) || (tzmin > tmax)) return false;
	if (tzmin > tmin) tmin = tzmin;
	if (tzmax < tmax) tmax = tzmax;

    /* Add a min, max distance for the ray (t0, t1)
    */
	return (tmax > t0);
}

} /* namespace xtcore */
