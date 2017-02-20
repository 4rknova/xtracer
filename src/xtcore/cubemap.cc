#include "cubemap.h"

#include <nmath/precision.h>
#include <nimg/color.h>

namespace xtracer {
    namespace assets {
        namespace textures {

int Cubemap::load(const char *file, CUBEMAP_FACE face)
{
    return m_textures[face].load(file);
}

nimg::ColorRGBAf Cubemap::sample(const NMath::Vector3f &direction) const
{
    NMath::Vector3f dir = direction.normalized();

    CUBEMAP_FACE face = CUBEMAP_FACE_TOP;
    float u = 0.f, v = 0.f;

    if ((nmath_abs(dir.x) >= nmath_abs(dir.y)) && (nmath_abs(dir.x) >= nmath_abs(dir.z)))
    {
        if      (dir.x > 0.0f) { face = CUBEMAP_FACE_RIGHT;  u =       (dir.z / dir.x+ 1.0f) * 0.5f; v =       (dir.y / dir.x+ 1.0f) * 0.5f; }
        else if (dir.x < 0.0f) { face = CUBEMAP_FACE_LEFT;   u =       (dir.z / dir.x+ 1.0f) * 0.5f; v = 1.f - (dir.y / dir.x+ 1.0f) * 0.5f; }
    }
    else if ((nmath_abs(dir.y) >= nmath_abs(dir.x)) && (nmath_abs(dir.y) >= nmath_abs(dir.z)))
    {
        if      (dir.y > 0.0f) { face = CUBEMAP_FACE_TOP;    u =       (dir.x / dir.y+ 1.0f) * 0.5f; v =       (dir.z / dir.y+ 1.0f) * 0.5f; }
        else if (dir.y < 0.0f) { face = CUBEMAP_FACE_BOTTOM; u = 1.f - (dir.x / dir.y+ 1.0f) * 0.5f; v = 1.f - (dir.z / dir.y+ 1.0f) * 0.5f; }
    }
    else if ((nmath_abs(dir.z) >= nmath_abs(dir.x)) && (nmath_abs(dir.z) >= nmath_abs(dir.y)))
    {
        if      (dir.z > 0.0f) { face = CUBEMAP_FACE_FRONT;  u = 1.f - (dir.x / dir.z+ 1.0f) * 0.5f; v =       (dir.y / dir.z+ 1.0f) * 0.5f; }
        else if (dir.z < 0.0f) { face = CUBEMAP_FACE_BACK;   u =       (dir.x / dir.z+ 1.0f) * 0.5f; v = 1.f - (dir.y / dir.z+ 1.0f) * 0.5f; }
    }

    return m_textures[face].sample(u, v);
}

        } /* namespace textures */
    } /* namespace assets */
} /* namespace xtracer */

