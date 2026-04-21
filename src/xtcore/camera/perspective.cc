#include <nmath/prng.h>
#include <cmath>
#include "perspective.h"

using nmath::RADIAN;
using nmath::scalar_t;
using nmath::prng_c;
using nmath::Vector3f;
using nmath::Matrix4x4f;

namespace xtcore {
    namespace camera {

Perspective::Perspective()
	: target(Vector3f(0,0,1))
	, up(Vector3f(0,1,0))
	, fov(XT_CAM_DEFAULT_FOV)
    , aperture(0)
    , flength(0)
    , aperture_blades(0)
    , aperture_rotation(0)
{}

namespace {

static inline Vector3f sample_aperture_disk(const scalar_t radius)
{
    const scalar_t u1 = prng_c(0.0f, 1.0f);
    const scalar_t u2 = prng_c(0.0f, 1.0f);
    const scalar_t r = std::sqrt(u1) * radius;
    const scalar_t theta = (scalar_t)(6.28318530717958647692) * u2;
    return Vector3f(r * std::cos(theta), r * std::sin(theta), 0.0f);
}

static inline Vector3f sample_aperture_polygon(const scalar_t radius, const int blades, const scalar_t rotation)
{
    if (blades < 3) return sample_aperture_disk(radius);

    const scalar_t u0 = prng_c(0.0f, 1.0f);
    const scalar_t u1 = prng_c(0.0f, 1.0f);
    const scalar_t u2 = prng_c(0.0f, 1.0f);

    int edge = (int)(u0 * (scalar_t)blades);
    if (edge >= blades) edge = blades - 1;

    const scalar_t step = (scalar_t)(6.28318530717958647692) / (scalar_t)blades;
    const scalar_t a0 = rotation + step * (scalar_t)edge;
    const scalar_t a1 = a0 + step;

    const Vector3f v0(radius * std::cos(a0), radius * std::sin(a0), 0.0f);
    const Vector3f v1(radius * std::cos(a1), radius * std::sin(a1), 0.0f);

    const scalar_t su = std::sqrt(u1);
    const scalar_t w0 = su * (1.0f - u2);
    const scalar_t w1 = su * u2;
    return v0 * w0 + v1 * w1;
}

} // namespace

const char* Perspective::get_type() const
{
    return "Perspective";
}

void Perspective::calculate_transform(Matrix4x4f &mat)
{
	Vector3f rz = (target - position).normalized();
	Vector3f rx = cross(rz, up).normalized();
	Vector3f ry = cross(rz, rx).normalized();

	mat = Matrix4x4f(rx.x, ry.x, rz.x, 0,
		                 rx.y, ry.y, rz.y, 0,
        		         rx.z, ry.z, rz.z, 0,
		                    0,    0,    0, 1);
}

Ray Perspective::get_primary_ray(float x, float y, float width, float height)
{
    // Keep the pre-transform direction unnormalized; DoF uses this camera-space ray.
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

    calculate_transform(m_transform);

	// Thin-lens DoF: sample a point on the lens disk and re-aim through focal plane.
    if (flength > 0 && aperture > 0) {
        const scalar_t half_aperture = aperture * 0.5f;
        const scalar_t aperture_rotation_rad = aperture_rotation * RADIAN;
        const Vector3f lens_point = sample_aperture_polygon(half_aperture, aperture_blades, aperture_rotation_rad);

        // Intersect the pinhole ray with focal plane z = flength in camera space.
        const scalar_t dz = ray.direction.z;
        if (std::fabs((double)dz) > (scalar_t)1e-8) {
            const scalar_t t_focus = flength / dz;
            const Vector3f focus_point = ray.direction * t_focus;
            ray.origin = lens_point;
            ray.direction = focus_point - lens_point;
        }
    }

	// Transform the direction vector
	ray.direction.transform(m_transform);
	ray.direction.normalize();

	// Transform the origin of the ray for DoF.
    if (flength > 0 && aperture > 0) {
		    ray.origin.transform(m_transform);
		    ray.origin += position;
    }

	return ray;
}

    } /* namespace camera */
} /* namespace xtcore */
