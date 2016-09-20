#ifndef XTRACER_CAM_PERSPECTIVE_H_INCLUDED
#define XTRACER_CAM_PERSPECTIVE_H_INCLUDED

#include <nmath/precision.h>
#include <nmath/vector.h>
#include <nmath/ray.h>
#include "camera.h"

#define XT_CAM_DEFAULT_FOV        M_PI / 4
#define XT_CAM_DEFAULT_PROJECTION CAM_PERSPECTIVE_PROJECTION_PERSPECTIVE
#define XT_CAM_DEFAULT_EYE        CAM_PERSPECTIVE_EYE_MONO
#define XT_CAM_DEFAULT_IPD         6.4f // 6.4 cm

class Camera : public xtracer::assets::ICamera
{
	public:
		NMath::Vector3f target;
		NMath::Vector3f up;
		NMath::scalar_t fov;
		NMath::scalar_t aperture;
		NMath::scalar_t flength;

		Camera();

		NMath::Ray get_primary_ray(float x, float y, float width, float height);
};

#endif /* XTRACER_CAM_PERSPECTIVE_H_INCLUDED */
