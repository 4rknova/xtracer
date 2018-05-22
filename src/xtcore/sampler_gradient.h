#ifndef XTCORE_SAMPLER_GRADIENT_H_INCLUDED
#define XTCORE_SAMPLER_GRADIENT_H_INCLUDED

#include <nmath/vector.h>
#include <nimg/color.h>
#include "sampler.h"

namespace xtcore {
    namespace sampler {

class Gradient : public ISampler
{
    public:
    nimg::ColorRGBf a;
    nimg::ColorRGBf b;

    nimg::ColorRGBf sample(const NMath::Vector3f &tc) const;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_GRADIENT_H_INCLUDED */
