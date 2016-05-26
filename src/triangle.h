#ifndef XTCORE_TRIANGLE_H_INCLUDED
#define XTCORE_TRIANGLE_H_INCLUDED

#include <nmath/precision.h>
#include <nmath/types.h>
#include <nmath/vector.h>

#include "defs.h"

#include "geometry.h"
#include "ray.h"

namespace NMath {

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

struct triangle_t { vec3_t v[3]; };
typedef struct triangle_t triangle_t;

static inline triangle_t triangle_pack(vec3_t v0, vec3_t v1, vec3_t v2);

#ifdef __cplusplus
}	/* __cplusplus */

class Triangle: public Geometry
{
    public:
        Triangle();

		bool intersection(const Ray &ray, IntInfo* i_info) const;
		void calc_aabb();
		Vector3f calc_normal() const;
		Vector3f calc_barycentric(const Vector3f &p) const;

        Vector3f v[3]; // position
        Vector3f n[3]; // normal
		Vector2f tc[3]; // texcoords
};

#endif	/* __cplusplus */

} /* namespace NMath */

#include "triangle.inl"

#endif /* XTCORE_TRIANGLE_H_INCLUDED */
