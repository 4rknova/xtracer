#include "cam_cubemap.h"

/* Cubemap Vertical Strip
** The faces are ordered as: +x -x +y -y +z -z
**/

using NMath::scalar_t;
using NMath::Vector3f;
using NMath::Matrix4x4f;

CamCubemap::CamCubemap()
{}

CamCubemap::~CamCubemap()
{}

NMath::Ray CamCubemap::get_primary_ray(float x, float y, float width, float height)
{
    NMath::Ray ray;

    y *= 6.f;
    size_t face = size_t(y) / size_t(height);

    ray.origin = position;
    ray.direction.x = (2.0 * (scalar_t)x / (scalar_t)width)  - 1.0;
    ray.direction.y = (2.0 * (scalar_t)y / (scalar_t)height) - 1.0;
    ray.direction.z = 1.0;

    ray.direction.normalize();

    Vector3f rx,ry,rz;

    switch(face) {
        case 0: rz = Vector3f(-1, 0, 0); ry = Vector3f( 0,-1, 0); rx = Vector3f( 0, 0, 1); break;
        case 1: rz = Vector3f(-1, 0, 0); ry = Vector3f( 0, 1, 0); rx = Vector3f( 0, 0,-1); break;

        case 2: rz = Vector3f( 0, 0, 1); ry = Vector3f( 0, 1, 0); rx = Vector3f( 1, 0, 0); break;
        case 3: rz = Vector3f( 0, 0, 1); ry = Vector3f( 0,-1, 0); rx = Vector3f( 1, 0, 0); break;

        case 4: rz = Vector3f( 0, 0, 1); ry = Vector3f( 0, 1, 0); rx = Vector3f( 1, 0, 0); break;
        case 5: rz = Vector3f( 0, 0, 1); ry = Vector3f( 0,-1, 0); rx = Vector3f( 1, 0, 0); break;
    }

    Matrix4x4f mat(rx.x, ry.x, rz.x, 0,
                   rx.y, ry.y, rz.y, 0,
                   rx.z, ry.z, rz.z, 0,
                      0,    0,    0, 1);

    ray.direction.transform(mat);
    ray.direction.normalize();

    return ray;
}
