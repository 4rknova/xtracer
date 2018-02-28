#ifndef NMATH_GEOMETRY_H_INCLUDED
#define NMATH_GEOMETRY_H_INCLUDED

#include "vector.h"
#include "aabb.h"
#include "ray.h"

namespace NMath {

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

enum XTCORE_GEOMETRY_TYPE
{
      GEOMETRY_PLANE
	, GEOMETRY_TRIANGLE
	, GEOMETRY_SPHERE
	, GEOMETRY_MESH		/* For external objects that might extend geometry */
};

enum XTCORE_GEOMETRY_LOCALITY
{
      XTCORE_GEOMETRY_STATIC
    , XTCORE_GEOMETRY_DYNAMIC
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

        virtual Vector3f point_sample() const = 0;
        virtual Ray ray_sample() const = 0;

		XTCORE_GEOMETRY_TYPE     type;
        XTCORE_GEOMETRY_LOCALITY locality;

		BoundingBox3 aabb;
		Vector2f uv_scale;
};

#endif	/* __cplusplus */

} /* namespace NMath */

#endif /* XTCORE_GEOMETRY_H_INCLUDED */
