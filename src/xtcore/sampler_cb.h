#ifndef XTRACER_SAMPLER_CB_H_INCLUDED
#define XTRACER_SAMPLER_CB_H_INCLUDED

#include "cubemap.h"
#include "sampler.h"

namespace xtracer {

class SamplerCubemap
{
    public:
    SamplerCubemap(
          const char *px, const char *nx
        , const char *py, const char *ny
        , const char *pz, const char *nz
    );

    nimg::ColorRGBf sample(NMath::Vector3f &uvw) const;

    private:
    xtracer::assets::Cubemap m_cubemap;
};

} /* namespace xtracer */

#endif /* XTRACER_SAMPLER_CB_H_INCLUDED */
