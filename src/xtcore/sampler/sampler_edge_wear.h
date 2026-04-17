#ifndef XTCORE_SAMPLER_EDGE_WEAR_H_INCLUDED
#define XTCORE_SAMPLER_EDGE_WEAR_H_INCLUDED

#include <xtcore/sampler.h>

namespace xtcore {
    namespace sampler {

class EdgeWear : public ISampler
{
    public:
    EdgeWear();
    virtual ~EdgeWear();

    virtual nimg::ColorRGBf sample(const nmath::Vector3f &uvw) const;

    nimg::ColorRGBf color_base;
    nimg::ColorRGBf color_worn;
    nmath::scalar_t scale;
    nmath::scalar_t sharpness;
    nmath::scalar_t coverage;
    int seed;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_EDGE_WEAR_H_INCLUDED */
