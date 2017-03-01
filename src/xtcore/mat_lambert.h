#ifndef XTRACER_MAT_LAMBERT_H_INCLUDED
#define XTRACER_MAT_LAMBERT_H_INCLUDED

#include <nmath/vector.h>
#include <nmath/intinfo.h>
#include <nimg/color.h>
#include "material.h"

namespace xtracer {
    namespace assets {

class MaterialLambert : public IMaterial
{
    public:
    virtual nimg::ColorRGBf shade(
          const NMath::Vector3f &cam_position
        , const NMath::Vector3f &light_position
        , const nimg::ColorRGBf &light_intensity
        , const NMath::IntInfo  &info) const;
};

    } /* namespace assets */
} /* namespace xtracer */

#endif /* XTRACER_MAT_LAMBERT_H_INCLUDED */
