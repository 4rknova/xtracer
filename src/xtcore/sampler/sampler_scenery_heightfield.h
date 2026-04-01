#ifndef XTCORE_SAMPLER_SCENERY_HEIGHTFIELD_H_INCLUDED
#define XTCORE_SAMPLER_SCENERY_HEIGHTFIELD_H_INCLUDED

#include <xtcore/sampler.h>

namespace xtcore {
    namespace sampler {

class SceneryHeightfield : public ISampler
{
    public:
    SceneryHeightfield();
    virtual ~SceneryHeightfield();

    virtual nimg::ColorRGBf sample(const nmath::Vector3f &uvw) const;

    int seed;
    nmath::scalar_t scale;
    int octaves;
    nmath::scalar_t lacunarity;
    nmath::scalar_t gain;
    nmath::scalar_t ridge_strength;
    nmath::scalar_t mountain_strength;
    nmath::scalar_t valley_strength;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_SCENERY_HEIGHTFIELD_H_INCLUDED */
