#ifndef XTCORE_ORTHOGRAPHIC_H_INCLUDED
#define XTCORE_ORTHOGRAPHIC_H_INCLUDED

#include <nmath/vector.h>
#include <nmath/matrix.h>
#include "math/ray.h"
#include "camera.h"

using nmath::Vector3f;
using nmath::Matrix4x4f;

namespace xtcore {
    namespace camera {

class Orthographic : public xtcore::asset::ICamera
{
    public:
        Vector3f target;
        Vector3f up;
        float    ortho_scale; // World-space width of the view volume

        Orthographic();
        const char* get_type() const;

        void calculate_transform(Matrix4x4f &mat);
        Ray get_primary_ray(float x, float y, float width, float height);

    private:
        Matrix4x4f m_transform;
};

    } /* namespace camera */
} /* namespace xtcore */

#endif /* XTCORE_ORTHOGRAPHIC_H_INCLUDED */
