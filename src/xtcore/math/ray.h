#ifndef XTCORE_RAY_H_INCLUDED
#define XTCORE_RAY_H_INCLUDED

#include <nmath/defs.h>
#include <nmath/types.h>
#include <nmath/vector.h>

using NMath::Vector3f;

namespace xtcore {

class Ray
{
    public:
        Ray();
        Ray(const Vector3f &org, const Vector3f &dir);

        Vector3f origin, direction;
};

} /* namespace xtcore */

#endif /* XTCORE_RAY_H_INCLUDED */
