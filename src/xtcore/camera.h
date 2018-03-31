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

#endif /* XTCORE_CAMERA_H_INCLUDED */
