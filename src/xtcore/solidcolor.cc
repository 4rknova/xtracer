#include "solidcolor.h"

namespace xtcore {
    namespace assets {

void SolidColor::set(nimg::ColorRGBf color)
{
    m_color = color;
}

nimg::ColorRGBf SolidColor::sample(const NMath::Vector3f &tc) const
{
    return m_color;
}

    } /* namespace assets */
} /* namespace xtcore */
