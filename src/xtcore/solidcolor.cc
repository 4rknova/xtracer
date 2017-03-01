#include "solidcolor.h"

namespace xtracer {
    namespace assets {

void SolidColor::set(nimg::ColorRGBAf &color)
{
    m_color = color;
}

nimg::ColorRGBAf SolidColor::sample(const NMath::Vector3f &tc) const
{
    return m_color;
}

    } /* namespace assets */
} /* namespace xtracer */
