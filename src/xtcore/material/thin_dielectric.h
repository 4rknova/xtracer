#ifndef XTCORE_MAT_THIN_DIELECTRIC_H_INCLUDED
#define XTCORE_MAT_THIN_DIELECTRIC_H_INCLUDED

#include <nmath/vector.h>
#include <nimg/color.h>
#include "math/hitrecord.h"
#include "material.h"

using nmath::Vector3f;
using nimg::ColorRGBf;
using xtcore::asset::ICamera;

namespace xtcore {
    namespace asset {
        namespace material {

class ThinDielectric : public xtcore::asset::IMaterial
{
    public:
    virtual bool shade(
                ColorRGBf    &intensity
        , const ICamera      *camera
        , const emitter_t    *emitter
        , const hit_record_t &hit_record) const;

    virtual bool sample_path(
                hit_result_t &hit_result
        , const hit_record_t &hit_record
    ) const;

    virtual bool bsdf_is_delta() const;
    virtual bool bsdf_eval(
                const hit_record_t &hit_record
        , const Vector3f &wo
        , const Vector3f &wi
        , ColorRGBf &f
        , scalar_t &pdf
    ) const;
    virtual bool bsdf_sample(
                const hit_record_t &hit_record
        , const Vector3f &wo
        , Vector3f &wi
        , ColorRGBf &f
        , scalar_t &pdf
    ) const;
};

        } /* namespace material */
    } /* namespace asset */
} /* namespace xtcore */

#endif /* XTCORE_MAT_THIN_DIELECTRIC_H_INCLUDED */
