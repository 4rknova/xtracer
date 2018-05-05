#ifndef XTCORE_MAT_EMISSIVE_H_INCLUDED
#define XTCORE_MAT_EMISSIVE_H_INCLUDED

#include <nmath/vector.h>
#include <nimg/color.h>
#include "math/hitrecord.h"
#include "material.h"

using NMath::Vector3f;
using nimg::ColorRGBf;
using xtcore::asset::ICamera;

namespace xtcore {
    namespace asset {
        namespace material {

class Emissive : public xtcore::asset::IMaterial
{
    public:
    virtual bool shade(
                ColorRGBf &intensity
        , const ICamera   *camera
        , const emitter_t *emitter
        , const HitRecord &info) const;

};

        } /* namespace material */
    } /* namespace asset */
} /* namespace xtcore */

#endif /* XTCORE_MAT_EMISSIVE_H_INCLUDED */
