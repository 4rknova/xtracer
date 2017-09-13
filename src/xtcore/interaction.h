#ifndef XT_INTERACTION_H_INCLUDED
#define XT_INTERACTION_H_INCLUDED

#include <nmath/vector.h>
#include <nimg/color.h>
#include "timestamp.h"

namespace xtracer {

struct surface_interaction_t
{
    Nimg::ColorRGBf intensity; // Reflected intensity
    NMath::Vector3f position;  // Position on surface
    NMath::Vector3f normal;    // Normal at position
    timestamp_t     time;      // Timestamp
};

} /* namespace xtracer */

#endif /*  XT_INTERACTION_H_INCLUDED */
