#ifndef XTCORE_CAMERA_H_INCLUDED
#define XTCORE_CAMERA_H_INCLUDED

#include <nmath/ray.h>

namespace xtcore {
    namespace assets {

class ICamera
{
	public:
        NMath::Vector3f position;

        virtual ~ICamera();
		virtual NMath::Ray get_primary_ray(float x, float y, float width, float height) = 0;
};

    } /* namespace assets */
} /* namespace xtcore */

#endif /* XTCORE_CAMERA_H_INCLUDED */
