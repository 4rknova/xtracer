#include <nmath/precision.h>
#include <nimg/color.h>
#include "sampler_erp.h"

namespace xtcore {
    namespace sampler {

int ERP::load(const char *file)
{
    return m_texture.load(file);
}

nimg::ColorRGBf ERP::sample(const NMath::Vector3f &tc) const
{
    NMath::Vector3f dir = tc.normalized();

    float u = ((acos(dir.z) / NMath::PI) + 1.f) * 0.5f;
    float v = atan2(dir.y, dir.x) / NMath::PI;

    NMath::Vector3f coords(u, v, 0);
    return m_texture.sample(coords);
}

    } /* namespace sampler */
} /* namespace xtcore */

