#ifndef XTCORE_CAM_ODS_H_INCLUDED
#define XTCORE_CAM_ODS_H_INCLUDED

#include <nmath/vector.h>
#include <nmath/ray.h>
#include "camera.h"

#define XT_CAM_DEFAULT_IPD 0.064f // 6.4 cm

class CamODS : public xtcore::assets::ICamera
{
	public:
    float           ipd;
    NMath::Vector3f orientation;

    CamODS();
    ~CamODS();
    NMath::Ray get_primary_ray(float x, float y, float width, float height);
};

#endif /* XTCORE_CAM_ODS_H_INCLUDED */
