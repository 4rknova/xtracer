#ifndef XTCORE_PERSPECTIVE_H_INCLUDED
#define XTCORE_PERSPECTIVE_H_INCLUDED

#include <nmath/vector.h>
#include "math/ray.h"
#include "camera.h"

#define XT_CAM_DEFAULT_FOV        M_PI / 4
#define XT_CAM_DEFAULT_PROJECTION PERSPECTIVE_PROJECTION_PERSPECTIVE
#define XT_CAM_DEFAULT_EYE        PERSPECTIVE_EYE_MONO

using NMath::Vector3f;

namespace xtcore {
    namespace camera {

class Perspective : public xtcore::asset::ICamera
{
	public:
		Vector3f target;
		Vector3f up;
		float    fov;
		float    aperture;
		float    flength;

		Perspective();

		Ray get_primary_ray(float x, float y, float width, float height);
};

    } /* namespace camera */
} /* namespace xtcore */

#endif /* XTCORE_PERSPECTIVE_H_INCLUDED */
