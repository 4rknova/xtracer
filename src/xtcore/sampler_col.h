#ifndef XTCORE_SAMPLER_COLOR_H_INCLUDED
#define XTCORE_SAMPLER_COLOR_H_INCLUDED

#include "sampler.h"

namespace xtcore {
    namespace sampler {

class SolidColor : public ISampler
{
    public:
    SolidColor();
    virtual ~SolidColor();

    void set(nimg::ColorRGBf &color);
    void get(nimg::ColorRGBf &color);
    virtual nimg::ColorRGBf sample(const nmath::Vector3f &uvw) const;

    private:
    nimg::ColorRGBf m_color;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_COLOR_H_INCLUDED */
