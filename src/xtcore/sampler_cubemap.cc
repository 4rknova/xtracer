#include <nmath/precision.h>
#include <nimg/color.h>
#include "sampler_cubemap.h"

namespace xtcore {
    namespace sampler {

int Cubemap::load(const char *file, CUBEMAP_FACE face)
{
    return m_textures[face].load(file);
}

nimg::ColorRGBf Cubemap::sample(const nmath::Vector3f &tc) const
{
    nmath::Vector3f dir = tc.normalized();

    CUBEMAP_FACE face = CUBEMAP_FACE_TOP;
    float u = 0.f, v = 0.f;

    if ((nmath_abs(dir.x) >= nmath_abs(dir.y)) && (nmath_abs(dir.x) >= nmath_abs(dir.z))) {
        if      (dir.x > 0.0f) { face = CUBEMAP_FACE_RIGHT;  u =     (dir.z / dir.x+ 1.0f) * 0.5f; v = 1.f - (dir.y / dir.x+ 1.0f) * 0.5f; }
        else if (dir.x < 0.0f) { face = CUBEMAP_FACE_LEFT;   u =     (dir.z / dir.x+ 1.0f) * 0.5f; v =       (dir.y / dir.x+ 1.0f) * 0.5f; }
    }
    else if ((nmath_abs(dir.y) >= nmath_abs(dir.x)) && (nmath_abs(dir.y) >= nmath_abs(dir.z))) {
        if      (dir.y > 0.0f) { face = CUBEMAP_FACE_TOP;    u =     (dir.x / dir.y+ 1.0f) * 0.5f; v = 1.f - (dir.z / dir.y+ 1.0f) * 0.5f; }
        else if (dir.y < 0.0f) { face = CUBEMAP_FACE_BOTTOM; u = 1.- (dir.x / dir.y+ 1.0f) * 0.5f; v = 1.f - (dir.z / dir.y+ 1.0f) * 0.5f; }
    }
    else if ((nmath_abs(dir.z) >= nmath_abs(dir.x)) && (nmath_abs(dir.z) >= nmath_abs(dir.y))) {
        if      (dir.z > 0.0f) { face = CUBEMAP_FACE_BACK;   u = 1.- (dir.x / dir.z+ 1.0f) * 0.5f; v = 1.f - (dir.y / dir.z+ 1.0f) * 0.5f; }
        else if (dir.z < 0.0f) { face = CUBEMAP_FACE_FRONT;  u = 1.- (dir.x / dir.z+ 1.0f) * 0.5f; v =       (dir.y / dir.z+ 1.0f) * 0.5f; }
    }

    nmath::Vector3f coords(u, v, 0);
    return m_textures[face].sample(coords);
}

    } /* namespace sampler */
} /* namespace xtcore */

