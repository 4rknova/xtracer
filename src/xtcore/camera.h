#ifndef XTCORE_CAMERA_H_INCLUDED
#define XTCORE_CAMERA_H_INCLUDED

#include "math/ray.h"

using NMath::Vector3f;

namespace xtcore {
    namespace asset {

class ICamera
{
	public:
        Vector3f position;

        virtual ~ICamera() {};
		virtual Ray get_primary_ray(float x, float y, float width, float height) = 0;
};

    } /* namespace asset */
} /* namespace xtcore */

#include "camera/perspective.h"
#include "camera/erp.h"
#include "camera/ods.h"
#include "camera/cubemap.h"

#endif /* XTCORE_CAMERA_H_INCLUDED */
