#ifndef NMATH_SPHERE_H_INCLUDED
#define NMATH_SPHERE_H_INCLUDED

#include "precision.h"
#include "vector.h"

#include "defs.h"

#include "geometry.h"
#include "ray.h"

//#define NMATH_USE_BBOX_INTERSECTION

namespace NMath {

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

struct sphere_t
{
    vec3_t origin;
    scalar_t radius;
};

typedef struct sphere_t sphere_t;

static inline sphere_t sphere_pack(vec3_t origin, scalar_t radius);

#ifdef __cplusplus
}	/* __cplusplus */

#define NMATH_SPHERE_DEFAULT_RADIUS 1.0

class Sphere: public Geometry
{
    public:
        Sphere();
        Sphere(const Vector3f &org, scalar_t rad);

		bool intersection(const Ray &ray, IntInfo* i_info) const;
		void calc_aabb();

        Vector3f point_sample() const;
        Ray ray_sample() const;

        Vector3f origin;
        scalar_t radius;
};

#endif	/* __cplusplus */

} /* namespace NMath */

#include "sphere.inl"

#endif /* NMATH_SPHERE_H_INCLUDED */
