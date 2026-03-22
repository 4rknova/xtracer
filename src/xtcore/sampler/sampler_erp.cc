#include <nmath/precision.h>
#include <nimg/color.h>
#include "sampler_erp.h"

namespace xtcore {
    namespace sampler {

int ERP::load(const char *file)
{
    return m_texture.load(file);
}

nimg::ColorRGBf ERP::sample(const nmath::Vector3f &tc) const
{
    nmath::Vector3f dir = tc.normalized();

    float theta = acos(dir.y);
    float phi   = atan2(dir.x, -dir.z);

    float u = 0.5f + phi / nmath::PI_DOUBLE;
    float v = theta / nmath::PI;

    nmath::Vector3f coords(u, v, 0);
    return m_texture.sample(coords);
}

    } /* namespace sampler */
} /* namespace xtcore */

