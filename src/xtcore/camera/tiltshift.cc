#include <nmath/prng.h>
#include <cmath>
#include "tiltshift.h"

using nmath::RADIAN;
using nmath::scalar_t;
using nmath::prng_c;
using nmath::Vector3f;
using nmath::Matrix4x4f;

namespace xtcore {
    namespace camera {

TiltShift::TiltShift()
    : target(Vector3f(0,0,1))
    , up(Vector3f(0,1,0))
    , fov(M_PI / 4)
    , aperture(0)
    , flength(0)
    , aperture_blades(0)
    , aperture_rotation(0)
    , tilt(0)
    , shift_x(0)
    , shift_y(0)
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

const char* TiltShift::get_type() const
{
    return "TiltShift";
}

void TiltShift::calculate_transform(Matrix4x4f &mat)
{
    Vector3f rz = (target - position).normalized();
    Vector3f rx = cross(up, rz).normalized();
    Vector3f ry = cross(rx, rz).normalized();

    mat = Matrix4x4f(rx.x, ry.x, rz.x, 0,
                     rx.y, ry.y, rz.y, 0,
                     rx.z, ry.z, rz.z, 0,
                        0,    0,    0, 1);
}

Ray TiltShift::get_primary_ray(float x, float y, float width, float height)
{
    Ray ray;

    const scalar_t aspect_ratio = (scalar_t)width / (scalar_t)height;
    ray.origin = position;

    // Film-plane ray direction with lens shift applied as a sensor-centre offset.
    // shift_x/shift_y are in normalised sensor units: 1.0 = full frame width/height.
    ray.direction.x = (2.0 * (scalar_t)x / (scalar_t)width)  - 1.0 + shift_x;
    ray.direction.y = ((2.0 * (scalar_t)y / (scalar_t)height) - 1.0 + shift_y) / aspect_ratio;
    ray.direction.z = 1.0 / tan(fov * RADIAN / 2.0);

    calculate_transform(m_transform);

    // Tilt-shift DoF: intersect the pinhole ray with the tilted focal plane, then
    // re-aim from a sampled point on the aperture disk through that focus point.
    //
    // The focal plane is tilted by `tilt` degrees around the camera's horizontal
    // axis (Scheimpflug principle).  In camera space its normal is:
    //   n = (0, -sin(tilt), cos(tilt))
    // and it passes through (0, 0, flength) on the optical axis.
    //
    // Solving dot(n, d*t - p0) = 0  for t gives:
    //   t = flength * cos(tilt) / (cos(tilt)*dz - sin(tilt)*dy)
    // For tilt = 0 this reduces to the standard  t = flength / dz.
    if (flength > 0 && aperture > 0) {
        const scalar_t tilt_rad = tilt * RADIAN;
        const scalar_t sin_t = std::sin(tilt_rad);
        const scalar_t cos_t = std::cos(tilt_rad);
        const scalar_t denom = cos_t * ray.direction.z - sin_t * ray.direction.y;

        if (std::fabs((double)denom) > (scalar_t)1e-8) {
            const scalar_t t_focus = (flength * cos_t) / denom;
            const Vector3f focus_point = ray.direction * t_focus;

            const scalar_t half_aperture = aperture * 0.5f;
            const scalar_t aperture_rotation_rad = aperture_rotation * RADIAN;
            const Vector3f lens_point = sample_aperture_polygon(half_aperture, aperture_blades, aperture_rotation_rad);

            ray.origin = lens_point;
            ray.direction = focus_point - lens_point;
        }
    }

    ray.direction.transform(m_transform);
    ray.direction.normalize();

    if (flength > 0 && aperture > 0) {
        ray.origin.transform(m_transform);
        ray.origin += position;
    }

    return ray;
}

    } /* namespace camera */
} /* namespace xtcore */
