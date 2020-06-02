#ifndef XTCORE_ERP_H_INCLUDED
#define XTCORE_ERP_H_INCLUDED

#include <nmath/vector.h>
#include "math/ray.h"
#include "camera.h"

using NMath::Vector3f;

namespace xtcore {
    namespace camera {

class ERP : public xtcore::asset::ICamera
{
	public:
    Vector3f orientation;
    
    const char* get_type() const;

    Ray get_primary_ray(float x, float y, float width, float height);
};

    } /* namespace camera */
} /* namespace xtcore */

#endif /* XTCORE_ERP_H_INCLUDED */
