#ifndef XTCORE_RAY_H_INCLUDED
#define XTCORE_RAY_H_INCLUDED

#include "nmath/types.h"
#include "nmath/vector.h"

#include "defs.h"

namespace NMath {

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

struct ray_t
{
	vec3_t origin, direction;
};

typedef struct ray_t ray_t;

static inline ray_t ray_pack(vec3_t origin, vec3_t direction);

#ifdef __cplusplus
}	/* __cplusplus */

class Ray
{
    public:
        Ray();                                          /* This relies on vector class default constructor setting all components to 0 */
        Ray(const Vector3f &org, const Vector3f &dir);

        Vector3f origin, direction;
};

#endif	/* __cplusplus */

} /* namespace NMath */

#include "ray.inl"

#endif /* XTCORE_RAY_H_INCLUDED */
