#ifndef XTRACER_CAMERA_HPP_INCLUDED
#define XTRACER_CAMERA_HPP_INCLUDED

#include <nmath/precision.h>
#include <nmath/vector.h>
#include <nmath/ray.h>
#include "defs.h"

using NMath::scalar_t;
using NMath::Vector3f;
using NMath::Ray;

#define XT_CAM_DEFAULT_FOV M_PI / 4

/* Pinhole camera */
class Camera
{
	public:
		Camera();
		Camera(Vector3f &pos, Vector3f &trg, Vector3f &upv, scalar_t fovx=XT_CAM_DEFAULT_FOV, scalar_t aprt=1.0, scalar_t flen=0.0, scalar_t shut=0.01);

		Ray get_primary_ray(float x, float y, float width, float height);
		Ray get_primary_ray_dof(float x, float y, float width, float height);

		Vector3f position;
		Vector3f target;
		Vector3f up;
		scalar_t fov;

		scalar_t aperture;
		scalar_t flength;
};

#endif /* XTRACER_CAMERA_HPP_INCLUDED */
