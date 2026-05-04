#include <cmath>
#include "orthographic.h"

using nmath::scalar_t;
using nmath::Vector3f;
using nmath::Matrix4x4f;

namespace xtcore {
    namespace camera {

Orthographic::Orthographic()
    : target(Vector3f(0, 0, 1))
    , up(Vector3f(0, 1, 0))
    , ortho_scale(1.0f)
{}

const char* Orthographic::get_type() const
{
    return "Orthographic";
}

void Orthographic::calculate_transform(Matrix4x4f &mat)
{
    Vector3f rz = (target - position).normalized();
    Vector3f rx = cross(rz, up).normalized();
    Vector3f ry = cross(rz, rx).normalized();

    mat = Matrix4x4f(rx.x, ry.x, rz.x, 0,
                     rx.y, ry.y, rz.y, 0,
                     rx.z, ry.z, rz.z, 0,
                        0,    0,    0, 1);
}

Ray Orthographic::get_primary_ray(float x, float y, float width, float height)
{
    Ray ray;

    scalar_t aspect_ratio = (scalar_t)width / (scalar_t)height;
    scalar_t halfW = ortho_scale * 0.5f;
    scalar_t halfH = halfW / aspect_ratio;

    scalar_t u = ((2.0f * (scalar_t)x / (scalar_t)width)  - 1.0f) * halfW;
    scalar_t v = ((2.0f * (scalar_t)y / (scalar_t)height) - 1.0f) * halfH;

    calculate_transform(m_transform);

    // All rays share the same direction (view forward)
    ray.direction = Vector3f(0, 0, 1);
    ray.direction.transform(m_transform);
    ray.direction.normalize();

    // Origin spans the view plane perpendicular to the view direction
    Vector3f offset(u, v, 0);
    offset.transform(m_transform);
    ray.origin = position + offset;

    return ray;
}

    } /* namespace camera */
} /* namespace xtcore */
