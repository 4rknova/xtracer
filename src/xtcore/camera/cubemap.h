#ifndef XTCORE_CUBEMAP_H_INCLUDED
#define XTCORE_CUBEMAP_H_INCLUDED

#include <nmath/vector.h>
#include "math/ray.h"
#include "camera.h"

namespace xtcore {
    namespace camera {

class Cubemap : public xtcore::asset::ICamera
{
	public:
    Ray get_primary_ray(float x, float y, float width, float height);
};

    } /* namespace camera */
} /* namespace xtcore */

#endif /* XTCORE_CUBEMAP_H_INCLUDED */
