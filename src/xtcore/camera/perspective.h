#ifndef XTCORE_PERSPECTIVE_H_INCLUDED
#define XTCORE_PERSPECTIVE_H_INCLUDED

#include <nmath/vector.h>
#include <nmath/matrix.h>
#include "math/ray.h"
#include "camera.h"

#define XT_CAM_DEFAULT_FOV        M_PI / 4
#define XT_CAM_DEFAULT_PROJECTION PERSPECTIVE_PROJECTION_PERSPECTIVE
#define XT_CAM_DEFAULT_EYE        PERSPECTIVE_EYE_MONO

using nmath::Vector3f;
using nmath::Matrix4x4f;

namespace xtcore {
    namespace camera {

class Perspective : public xtcore::asset::ICamera
{
	public:
		Vector3f target;
		Vector3f up;
		float    fov;
		float    aperture;
		float    flength;

		Perspective();
        const char* get_type() const;

        void calculate_transform(Matrix4x4f &mat);
		Ray get_primary_ray(float x, float y, float width, float height);

    private:
        Matrix4x4f m_transform;
};

    } /* namespace camera */
} /* namespace xtcore */

#endif /* XTCORE_PERSPECTIVE_H_INCLUDED */
