#ifndef XTRACER_SAMPLER_COLOR_H_INCLUDED
#define XTRACER_SAMPLER_COLOR_H_INCLUDED

#include "sampler.h"

namespace xtracer {

class SamplerColor
{
    public:
    SamplerColor(nimg::ColorRGBf &color);

    nimg::ColorRGBf sample(NMath::Vector3f &uvw) const;

    private:
    nimg::ColorRGBf m_color;
};

} /* namespace xtracer */

#endif /* XTRACER_SAMPLER_COLOR_H_INCLUDED */
