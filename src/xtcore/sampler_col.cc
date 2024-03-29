#include "macro.h"
#include "sampler_col.h"

namespace xtcore {
    namespace sampler {

SolidColor::SolidColor()
{}

SolidColor::~SolidColor()
{}

void SolidColor::get(nimg::ColorRGBf &color)
{
    color = m_color;
}

void SolidColor::set(nimg::ColorRGBf &color)
{
    m_color = color;
}

nimg::ColorRGBf SolidColor::sample(const nmath::Vector3f &uvw) const
{
    UNUSED(uvw)
    return m_color;
}

    } /* namespace sampler */
} /* namespace xtcore */
