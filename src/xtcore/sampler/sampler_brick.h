#ifndef XTCORE_SAMPLER_BRICK_H_INCLUDED
#define XTCORE_SAMPLER_BRICK_H_INCLUDED

#include <xtcore/sampler.h>

namespace xtcore {
    namespace sampler {

class Brick : public ISampler
{
    public:
    Brick();
    virtual ~Brick();

    virtual nimg::ColorRGBf sample(const nmath::Vector3f &uvw) const;

    nimg::ColorRGBf color_brick;
    nimg::ColorRGBf color_mortar;
    nmath::scalar_t scale_u;
    nmath::scalar_t scale_v;
    nmath::scalar_t mortar_u;
    nmath::scalar_t mortar_v;
    nmath::scalar_t color_variation;
    int seed;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_BRICK_H_INCLUDED */
