#ifndef XTCORE_GEOMETRY_H_INCLUDED
#define XTCORE_GEOMETRY_H_INCLUDED

#include <nmath/vector.h>
#include <nmath/aabb.h>

#include "ray.h"

namespace NMath {

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

enum XTCORE_GEOMETRY_TYPE
{
	GEOMETRY_PLANE,
	GEOMETRY_TRIANGLE,
	GEOMETRY_SPHERE,
	GEOMETRY_MESH		/* For external objects that might extend geometry */
};

#ifdef __cplusplus
}	/* __cplusplus */

// forward declaration
class IntInfo;

class Geometry
{
    public:
		Geometry(XTCORE_GEOMETRY_TYPE t);
		virtual ~Geometry(); 
		virtual bool intersection(const Ray &ray, IntInfo* i_info) const = 0;
		virtual void calc_aabb() = 0;

		XTCORE_GEOMETRY_TYPE type;

		BoundingBox3 aabb;

		Vector2f uv_scale;
};

#endif	/* __cplusplus */

} /* namespace NMath */

#endif /* XTCORE_GEOMETRY_H_INCLUDED */
