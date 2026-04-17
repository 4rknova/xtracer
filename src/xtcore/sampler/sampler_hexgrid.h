#ifndef XTCORE_SAMPLER_HEXGRID_H_INCLUDED
#define XTCORE_SAMPLER_HEXGRID_H_INCLUDED

#include <xtcore/sampler.h>

namespace xtcore {
    namespace sampler {

class HexGrid : public ISampler
{
    public:
    HexGrid();
    virtual ~HexGrid();

    virtual nimg::ColorRGBf sample(const nmath::Vector3f &uvw) const;

    nimg::ColorRGBf color_tile;
    nimg::ColorRGBf color_border;
    nmath::scalar_t scale;
    nmath::scalar_t border_width;
    nmath::scalar_t border_softness;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_HEXGRID_H_INCLUDED */
