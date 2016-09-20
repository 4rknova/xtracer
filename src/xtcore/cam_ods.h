#ifndef XTRACER_CAM_ODS_H_INCLUDED
#define XTRACER_CAM_ODS_H_INCLUDED

#include <nmath/vector.h>
#include <nmath/ray.h>
#include "camera.h"

#define XT_CAM_DEFAULT_IPD 0.064f // 6.4 cm

class CamODS : public xtracer::assets::ICamera
{
	public:
    NMath::scalar_t ipd;

    CamODS();
    NMath::Ray get_primary_ray(float x, float y, float width, float height);
};

#endif /* XTRACER_CAM_ODS_H_INCLUDED */
