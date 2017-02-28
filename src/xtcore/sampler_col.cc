#include "sampler_col.h"

namespace xtracer {

SamplerColor::SamplerColor(nimg::ColorRGBf &color)
    : m_color(color)
{}

nimg::ColorRGBf SamplerColor::sample(NMath::Vector3f &uvw) const
{
    return m_color;
}

} /* namespace xtracer */
