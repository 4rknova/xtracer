#include "sampler_col.h"

namespace xtcore {
    namespace assets {

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

nimg::ColorRGBf SolidColor::sample(const NMath::Vector3f &uvw) const
{
    return m_color;
}

    } /* namespace assets */
} /* namespace xtcore */
