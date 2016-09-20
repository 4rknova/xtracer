#ifndef XTRACER_CAMERA_H_INCLUDED
#define XTRACER_CAMERA_H_INCLUDED

#include <nmath/ray.h>

namespace xtracer {
    namespace assets {

class ICamera
{
	public:
        NMath::Vector3f position;

        virtual ~ICamera();
		virtual NMath::Ray get_primary_ray(float x, float y, float width, float height) = 0;
};

    } /* namespace assets */
} /* namespace xtracer */

#endif /* XTRACER_CAMERA_H_INCLUDED */
