#ifndef XTRACER_SOLIDCOLOR_H_INCLUDED
#define XTRACER_SOLIDCOLOR_H_INCLUDED

#include "sampler.h"

namespace xtracer {
    namespace assets {

class SolidColor : public ISampler
{
    public:
    void set(nimg::ColorRGBf color);

    nimg::ColorRGBf sample(const NMath::Vector3f &tc) const;

    private:
    nimg::ColorRGBf m_color;
};

    } /* namespace assets */
} /* namespace xtracer */

#endif /* XTRACER_SOLIDCOLOR_H_INCLUDED */
