#ifndef XTCORE_ODS_H_INCLUDED
#define XTCORE_ODS_H_INCLUDED

#include <nmath/vector.h>
#include "math/ray.h"
#include "camera.h"

#define XT_CAM_DEFAULT_IPD 0.064f // 6.4 cm

using NMath::Vector3f;

namespace xtcore {
    namespace camera {

class ODS : public xtcore::asset::ICamera
{
	public:
    float    ipd;
    Vector3f orientation;

    ODS();

    Ray get_primary_ray(float x, float y, float width, float height);
};

    } /* namespace camera */
} /* namespace xtcore */

#endif /* XTCORE_ODS_H_INCLUDED */
