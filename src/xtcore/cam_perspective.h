#ifndef XTCORE_CAM_PERSPECTIVE_H_INCLUDED
#define XTCORE_CAM_PERSPECTIVE_H_INCLUDED

#include <nmath/precision.h>
#include <nmath/vector.h>
#include <nmath/ray.h>
#include "camera.h"

#define XT_CAM_DEFAULT_FOV        M_PI / 4
#define XT_CAM_DEFAULT_PROJECTION CAM_PERSPECTIVE_PROJECTION_PERSPECTIVE
#define XT_CAM_DEFAULT_EYE        CAM_PERSPECTIVE_EYE_MONO

class CamPerspective : public xtcore::assets::ICamera
{
	public:
		NMath::Vector3f target;
		NMath::Vector3f up;
		float           fov;
		float           aperture;
		float           flength;

		CamPerspective();
		~CamPerspective();

		NMath::Ray get_primary_ray(float x, float y, float width, float height);
};

#endif /* XTCORE_CAM_PERSPECTIVE_H_INCLUDED */
