#include "nmath/vector.h"
#include "ray.h"

namespace NMath {

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

#ifdef __cplusplus
}

Ray::Ray(const Vector3f &org, const Vector3f &dir)
    : origin(org), direction(dir.normalized())
{}

Ray::Ray()
	: origin(Vector3f(0,0,0)), direction(Vector3f(0,0,1))
{}

#endif	/* __cplusplus */

} /* namespace NMath */
