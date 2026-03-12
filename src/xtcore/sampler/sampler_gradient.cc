#include "sampler_gradient.h"

namespace xtcore {
    namespace sampler {

/* Test colors
** a: (1.0,1.0,1.0)
** b: (0.5,0.7,1.0)
*/

nimg::ColorRGBf Gradient::sample(const nmath::Vector3f &tc) const
{
    nmath::Vector3f dir = tc.normalized();

    float t = 0.5f * dir.y + 1.0; // remap to [0, 1]
    return (1.0 - t) * a + t * b;
}

    } /* namespace sampler */
} /* namespace xtcore */

