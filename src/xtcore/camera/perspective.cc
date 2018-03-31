#include <nmath/matrix.h>
#include <nmath/prng.h>
#include "perspective.h"

using NMath::RADIAN;
using NMath::scalar_t;
using NMath::prng_c;
using NMath::Vector3f;
using NMath::Matrix4x4f;

namespace xtcore {
    namespace camera {

Perspective::Perspective()
	: target(Vector3f(0,0,1))
	, up(Vector3f(0,1,0))
	, fov(XT_CAM_DEFAULT_FOV)
    , aperture(0)
    , flength(0)
{}

Ray Perspective::get_primary_ray(float x, float y, float width, float height)
{
    // Note that the direction vector of ray is not normalized. 
    // The DoF ray calculation depends on this at the moment.
    Ray ray;

    scalar_t aspect_ratio = (scalar_t)width / (scalar_t)height;
    ray.origin = position;

	// Calculate the ray's intersection point on the projection plane.
	ray.direction.x = (2.0 * (scalar_t)x / (scalar_t)width) - 1.0;
	ray.direction.y = ((2.0 * (scalar_t)y / (scalar_t)height) - 1.0) / aspect_ratio;
	ray.direction.z = 1.0 / tan(fov * RADIAN / 2.0);

	/*
		Setting up the look-at matrix is easy when you consider that a matrix
		is basically a rotated unit cube formed by three vectors (the 3x3 part) at a
		particular position (the 1x3 part).

		We already have one of the three vectors:
			-	The z-axis of the matrix is simply the view direction.
			-	The x-axis of the matrix is a bit tricky: if the camera is not tilted,
				then the x-axis of the matrix is perpendicular to the z-axis and
				the vector (0, 1, 0).
			-	The y-axis is perpendicular to the other two, so we simply calculate
				the cross product of the x-axis and the z-axis to obtain the y-axis.
				Note that the y-axis is calculated using the reversed z-axis. The
				image will be upside down without this adjustment.
	*/

	// Calculate the camera direction vector and normalize it.
	Vector3f camdir = target - position;
	camdir.normalize();

	Vector3f rx,ry,rz;

	rz = camdir;
	rx = cross(up, rz);
	rx.normalize();
	ry = cross(rx, rz);
	ry.normalize();

	Matrix4x4f mat(rx.x, ry.x, rz.x, 0,
		           rx.y, ry.y, rz.y, 0,
        		   rx.z, ry.z, rz.z, 0,
		              0,    0,    0, 1);


	// Calculate the deviated ray direction for DoF
    if (flength > 0) {
        Ray fray;
    	fray.origin = ray.direction;
    	scalar_t half_aperture = aperture / 2.f;
    	fray.origin.x += prng_c(-half_aperture, half_aperture);
    	fray.origin.y += prng_c(-half_aperture, half_aperture);

        // Find the intersection point on the focal plane
    	Vector3f fpip = ray.direction + flength * ray.direction.normalized();
    	fray.direction = fpip - fray.origin;

        ray = fray;
    }

	// Transform the direction vector
	ray.direction.transform(mat);
	ray.direction.normalize();

	// Transform the origin of the ray for DoF
    if (flength > 0) {
	    ray.origin.transform(mat);
	    ray.origin += position;
    }

	return ray;
}

    } /* namespace camera */
} /* namespace xtcore */
