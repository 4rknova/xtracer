#ifndef XTCORE_SAMPLER_STARS_H_INCLUDED
#define XTCORE_SAMPLER_STARS_H_INCLUDED

#include <xtcore/sampler.h>

namespace xtcore {
    namespace sampler {

class Stars : public ISampler
{
    public:
    Stars();
    virtual ~Stars();

    virtual nimg::ColorRGBf sample(const nmath::Vector3f &uvw) const;

    nimg::ColorRGBf background_color;
    nmath::scalar_t density;
    nmath::scalar_t min_brightness;
    nmath::scalar_t max_brightness;
    nmath::scalar_t star_size;
    int seed;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_STARS_H_INCLUDED */
