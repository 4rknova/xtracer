#ifndef XTCORE_SURFACE_H_INCLUDED
#define XTCORE_SURFACE_H_INCLUDED

#include <nmath/vector.h>
#include "math/hitrecord.h"
#include "aabb.h"
#include "ray.h"

namespace xtcore {
    namespace asset {

class ISurface
{
public:
    ISurface();
	virtual ~ISurface();

	virtual bool intersection(const Ray &ray, hit_record_t* i_info) const = 0;
    virtual NMath::scalar_t distance(NMath::Vector3f p) const;
	virtual void calc_aabb() = 0;

    virtual Vector3f point_sample() const = 0;
    virtual Ray ray_sample() const = 0;

	BoundingBox3 aabb;
	Vector2f uv_scale;
};

    } /* namespace asset */
} /* namespace xtcore */

#endif /* XTCORE_SURFACE_H_INCLUDED */
