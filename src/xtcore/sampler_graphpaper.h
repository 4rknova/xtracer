#ifndef XTCORE_SAMPLER_GRAPHPAPER_H_INCLUDED
#define XTCORE_SAMPLER_GRAPHPAPER_H_INCLUDED

#include "sampler.h"

namespace xtcore {
    namespace sampler {

class GraphPaper : public ISampler
{
    public:
    GraphPaper();
    virtual ~GraphPaper();

    virtual nimg::ColorRGBf sample(const nmath::Vector3f &uvw) const;

    nimg::ColorRGBf base_color;
    nimg::ColorRGBf minor_color;
    nimg::ColorRGBf major_color;
    nmath::scalar_t scale;
    nmath::scalar_t minor_width;
    nmath::scalar_t major_width;
    int major_every;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_GRAPHPAPER_H_INCLUDED */
