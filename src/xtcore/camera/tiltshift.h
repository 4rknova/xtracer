#ifndef XTCORE_TILTSHIFT_H_INCLUDED
#define XTCORE_TILTSHIFT_H_INCLUDED

#include <nmath/vector.h>
#include <nmath/matrix.h>
#include "math/ray.h"
#include "camera.h"

using nmath::Vector3f;
using nmath::Matrix4x4f;

namespace xtcore {
    namespace camera {

class TiltShift : public xtcore::asset::ICamera
{
    public:
        Vector3f target;
        Vector3f up;
        float    fov;
        float    aperture;
        float    flength;
        int      aperture_blades;
        float    aperture_rotation;
        float    tilt;    // Tilt angle in degrees — rotates the focal plane around the camera's horizontal axis (Scheimpflug)
        float    shift_x; // Horizontal lens shift in normalised sensor units (1.0 = full frame width)
        float    shift_y; // Vertical lens shift in normalised sensor units (1.0 = full frame height)

        TiltShift();
        const char* get_type() const;

        void calculate_transform(Matrix4x4f &mat);
        Ray get_primary_ray(float x, float y, float width, float height);

    private:
        Matrix4x4f m_transform;
};

    } /* namespace camera */
} /* namespace xtcore */

#endif /* XTCORE_TILTSHIFT_H_INCLUDED */
