#ifndef XTCORE_CAM_ERP_H_INCLUDED
#define XTCORE_CAM_ERP_H_INCLUDED

#include <nmath/vector.h>
#include <nmath/ray.h>
#include "camera.h"

class CamERP : public xtcore::assets::ICamera
{
	public:
    NMath::Vector3f orientation;

    CamERP();
    ~CamERP();
    NMath::Ray get_primary_ray(float x, float y, float width, float height);
};

#endif /* XTCORE_CAM_ERP_H_INCLUDED */
