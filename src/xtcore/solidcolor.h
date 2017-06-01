#ifndef XTCORE_SOLIDCOLOR_H_INCLUDED
#define XTCORE_SOLIDCOLOR_H_INCLUDED

#include "sampler.h"

namespace xtcore {
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
} /* namespace xtcore */

#endif /* XTCORE_SOLIDCOLOR_H_INCLUDED */
