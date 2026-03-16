#ifndef XTCORE_SAMPLER_VORONOI_NORMAL_H_INCLUDED
#define XTCORE_SAMPLER_VORONOI_NORMAL_H_INCLUDED

#include <xtcore/sampler.h>

namespace xtcore {
    namespace sampler {

class VoronoiNormal : public ISampler
{
    public:
    VoronoiNormal();
    virtual ~VoronoiNormal();

    virtual nimg::ColorRGBf sample(const nmath::Vector3f &uvw) const;

    int cells;
    nmath::scalar_t max_deviation;
    int seed;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_VORONOI_NORMAL_H_INCLUDED */
