#ifndef XTRACER_CAM_ERP_H_INCLUDED
#define XTRACER_CAM_ERP_H_INCLUDED

#include <nmath/vector.h>
#include <nmath/ray.h>
#include "camera.h"

class CamERP : public xtracer::assets::ICamera
{
	public:
    NMath::Vector3f orientation;

    CamERP();
    ~CamERP();
    NMath::Ray get_primary_ray(float x, float y, float width, float height);
};

#endif /* XTRACER_CAM_ERP_H_INCLUDED */
