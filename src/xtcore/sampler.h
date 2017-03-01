#ifndef XTRACER_SAMPLER_H_INCLUDED
#define XTRACER_SAMPLER_H_INCLUDED

#include <nmath/vector.h>
#include <nimg/color.h>

namespace xtracer {
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
    virtual nimg::ColorRGBAf sample(const NMath::Vector3f &uvw) const = 0;

    private:
    FILTERING m_filtering;
};

    } /* namespace assets */
} /* namespace xtracer */


#endif /* XTRACER_SAMPLER_H_INCLUDED */
