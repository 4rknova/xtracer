#ifndef XTCORE_MAT_BLINN_PHONG_H_INCLUDED
#define XTCORE_MAT_BLINN_PHONG_H_INCLUDED

#include <nmath/vector.h>
#include <nimg/color.h>
#include "math/hitrecord.h"
#include "material.h"

using NMath::Vector3f;
using nimg::ColorRGBf;

namespace xtcore {
    namespace material {

class BlinnPhong : public xtcore::asset::IMaterial
{
    public:
    ColorRGBf shade(const Vector3f  &cam_position
                  , const Vector3f  &light_position
                  , const ColorRGBf &light_intensity
                  , const HitRecord &info) const;
};

    } /* namespace material */
} /* namespace xtcore */

#endif /* XTCORE_MAT_BLINN_PHONG_H_INCLUDED */