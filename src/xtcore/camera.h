#ifndef XTRACER_CAMERA_H_INCLUDED
#define XTRACER_CAMERA_H_INCLUDED

#include <nmath/precision.h>
#include <nmath/vector.h>
#include <nmath/ray.h>

using NMath::scalar_t;
using NMath::Vector3f;
using NMath::Matrix4x4f;
using NMath::Ray;

#define XT_CAM_DEFAULT_FOV        M_PI / 4
#define XT_CAM_DEFAULT_PROJECTION CAMERA_PROJECTION_PERSPECTIVE
#define XT_CAM_DEFAULT_EYE        CAMERA_EYE_STEREO_LEFT
#define XT_CAM_DEFAULT_IPD         6.4f // 6.4 cm

enum CAMERA_EYE {
     CAMERA_EYE_MONO
   , CAMERA_EYE_STEREO_LEFT
   , CAMERA_EYE_STEREO_RIGHT
};

enum CAMERA_PROJECTION {
     CAMERA_PROJECTION_PERSPECTIVE
   , CAMERA_PROJECTION_ODS
};

class Camera
{
	public:
		Vector3f          position;
		Vector3f          target;
		Vector3f          up;
		scalar_t          fov;
		scalar_t          aperture;
		scalar_t          flength;
        scalar_t          ipd;
        CAMERA_PROJECTION projection;
        CAMERA_EYE        eye;

		Camera();

		Ray get_primary_ray(float x, float y, float width, float height);

   private:
        Ray get_primary_ray_ods         (float x, float y, float width, float height);
        Ray get_primary_ray_perspective (float x, float y, float width, float height);

        Ray get_perspective_ray(float x, float y, float width, float height);
        Matrix4x4f get_transformation_matrix();
};

#endif /* XTRACER_CAMERA_H_INCLUDED */
