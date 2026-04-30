#ifndef XTCORE_SAMPLER_SCALEGRID_H_INCLUDED
#define XTCORE_SAMPLER_SCALEGRID_H_INCLUDED

#include <xtcore/sampler.h>

namespace xtcore {
    namespace sampler {

class ScaleGrid : public ISampler
{
    public:
    ScaleGrid();
    virtual ~ScaleGrid();

    virtual nimg::ColorRGBf sample(const nmath::Vector3f &uvw) const;

    nimg::ColorRGBf base_color;
    nimg::ColorRGBf line_color;
    nimg::ColorRGBf text_color;
    nmath::scalar_t scale;
    nmath::scalar_t line_width;
    nmath::scalar_t text_size;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_SCALEGRID_H_INCLUDED */
