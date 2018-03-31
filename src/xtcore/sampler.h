#ifndef XTCORE_SAMPLER_H_INCLUDED
#define XTCORE_SAMPLER_H_INCLUDED

#include <nmath/vector.h>
#include <nimg/color.h>

using NMath::Vector3f;
using nimg::ColorRGBf;

namespace xtcore {
    namespace sampler {

enum FILTERING
{
      FILTERING_NEAREST
    , FILTERING_BILINEAR
};

class ISampler
{
    public:
             ISampler();
    virtual ~ISampler();
    virtual ColorRGBf sample(const Vector3f &uvw) const = 0;

    FILTERING filtering;
};

    } /* namespace sampler */
} /* namespace xtcore */


#endif /* XTCORE_SAMPLER_H_INCLUDED */
