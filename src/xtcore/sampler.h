#ifndef XTCORE_SAMPLER_H_INCLUDED
#define XTCORE_SAMPLER_H_INCLUDED

#include <nmath/vector.h>
#include <nimg/color.h>

namespace xtcore {
    namespace assets {

enum FILTERING
{
      FILTERING_NEAREST
    , FILTERING_BILINEAR
};

class ISampler
{
    public:
    virtual ~ISampler();
    virtual nimg::ColorRGBf sample(const NMath::Vector3f &uvw) const = 0;

    private:
    FILTERING m_filtering;
};

    } /* namespace assets */
} /* namespace xtcore */


#endif /* XTCORE_SAMPLER_H_INCLUDED */
