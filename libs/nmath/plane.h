#ifndef XTCORE_PLANE_H_INCLUDED
#define XTCORE_PLANE_H_INCLUDED

#include <nmath/precision.h>
#include <nmath/vector.h>

#include "defs.h"
#include "geometry.h"
#include "ray.h"

namespace NMath {

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

struct plane_t
{
    vec3_t normal;
    scalar_t distance;
};

typedef struct plane_t plane_t;

static inline plane_t plane_pack(vec3_t normal, scalar_t distance);

#ifdef __cplusplus
}	/* __cplusplus */

#define XTCORE_PLANE_DEFAULT_DISTANCE 1.0

class Plane: public Geometry
{
	public:
		Plane();

		bool intersection(const Ray &ray, IntInfo* i_info) const;
		void calc_aabb();

        Vector3f point_sample() const;
        Ray ray_sample() const;

		Vector3f normal;
		scalar_t distance;
};

#endif	/* __cplusplus */

} /* namespace NMath */

#include "plane.inl"

#endif /* XTCORE_PLANE_H_INCLUDED */
