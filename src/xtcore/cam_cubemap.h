#ifndef XTCORE_CAM_CUBEMAP_H_INCLUDED
#define XTCORE_CAM_CUBEMAP_H_INCLUDED

#include <nmath/vector.h>
#include <nmath/ray.h>
#include "camera.h"

class CamCubemap : public xtcore::assets::ICamera
{
	public:
    CamCubemap();
    ~CamCubemap();
    NMath::Ray get_primary_ray(float x, float y, float width, float height);
};

#endif /* XTCORE_CAM_CUBEMAP_H_INCLUDED */
