#include "cubemap.h"

/* Cubemap Vertical Strip
** The faces are ordered as: +x -x +y -y +z -z
**/

using nmath::scalar_t;
using nmath::Vector3f;
using nmath::Matrix4x4f;

namespace xtcore {
    namespace camera {

const char* Cubemap::get_type() const
{
    return "Cubemap Projection";
}

Ray Cubemap::get_primary_ray(float x, float y, float width, float height)
{
    Ray ray;

    size_t face_size = (size_t)(height) / 6;
    size_t face      = (size_t)(y) / face_size;

    Vector3f rx, ry, rz, up, camdir;

    switch(face) {
        case 0: camdir = Vector3f( 1, 0, 0); up = Vector3f(0, 1, 0); break;
        case 1: camdir = Vector3f(-1, 0, 0); up = Vector3f(0, 1, 0); break;
        case 2: camdir = Vector3f( 0, 1, 0); up = Vector3f(0, 0,-1); break;
        case 3: camdir = Vector3f( 0,-1, 0); up = Vector3f(0, 0, 1); break;
        case 4: camdir = Vector3f( 0, 0, 1); up = Vector3f(0, 1, 0); break;
        case 5: camdir = Vector3f( 0, 0,-1); up = Vector3f(0, 1, 0); break;
    }

    y -= (float)(face * face_size);

    ray.origin = position;
    ray.direction.x = (2.f * (scalar_t)x / (scalar_t)width    ) - 1.f;
    ray.direction.y = (2.f * (scalar_t)y / (scalar_t)face_size) - 1.f;
    ray.direction.z = 1.f;
    ray.direction.normalize();

    rz = camdir;
    rx = cross(up, rz);
    rx.normalize();
    ry = cross(rx, rz);
    ry.normalize();

    Matrix4x4f mat(rx.x, ry.x, rz.x, 0,
                   rx.y, ry.y, rz.y, 0,
                   rx.z, ry.z, rz.z, 0,
                      0,    0,    0, 1);

    ray.direction.transform(mat);
    ray.direction.normalize();

    return ray;
}

    } /* namespace camera */
} /* namespace xtcore */
