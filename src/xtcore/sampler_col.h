#ifndef XTCORE_SAMPLER_COLOR_H_INCLUDED
#define XTCORE_SAMPLER_COLOR_H_INCLUDED

#include "sampler.h"

namespace xtcore {

class SamplerColor
{
    public:
    SamplerColor(nimg::ColorRGBf &color);

    nimg::ColorRGBf sample(NMath::Vector3f &uvw) const;

    private:
    nimg::ColorRGBf m_color;
};

} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_COLOR_H_INCLUDED */
