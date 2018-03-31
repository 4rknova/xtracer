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
	ray - axis aligned bounding box intersection test based on:
	"An Efficient and Robust Ray-Box Intersection Algorithm",
	Amy Williams, Steve Barrus, R. Keith Morley, and Peter Shirley
	Journal of graphics tools, 10(1):49-54, 2005
*/

bool BoundingBox3::intersection(const Ray &ray) const
{
	if (ray.origin > min && ray.origin < max)
		return true;

	Vector3f aabb[2] = {min, max};
	static const double t0 = 0.0;

	int xsign = (int)(ray.direction.x < 0.0);
	double invdirx = 1.0 / ray.direction.x;

	double tmin = (aabb[xsign].x - ray.origin.x) * invdirx;
	double tmax = (aabb[1 - xsign].x - ray.origin.x) * invdirx;

	int ysign = (int)(ray.direction.y < 0.0);
	double invdiry = 1.0 / ray.direction.y;
	double tymin = (aabb[ysign].y - ray.origin.y) * invdiry;
	double tymax = (aabb[1 - ysign].y - ray.origin.y) * invdiry;

	if ((tmin > tymax) || (tymin > tmax))
		return false;

	if (tymin > tmin) tmin = tymin;
	if (tymax < tmax) tmax = tymax;

	int zsign = (int)(ray.direction.z < 0.0);
	double invdirz = 1.0 / ray.direction.z;
	double tzmin = (aabb[zsign].z - ray.origin.z) * invdirz;
	double tzmax = (aabb[1 - zsign].z - ray.origin.z) * invdirz;

	if ((tmin > tzmax) || (tzmin > tmax))
		return false;

	if (tzmin > tmin) tmin = tzmin;
	if (tzmax < tmax) tmax = tzmax;

	return (tmax > t0);
}

} /* namespace xtcore */
