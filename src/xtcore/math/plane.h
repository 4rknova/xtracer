#ifndef XTCORE_PLANE_H_INCLUDED
#define XTCORE_PLANE_H_INCLUDED

#include <nmath/defs.h>
#include <nmath/precision.h>
#include <nmath/vector.h>
#include "surface.h"
#include "ray.h"

#define XTCORE_PLANE_DEFAULT_DISTANCE 1.0

using NMath::scalar_t;
using NMath::Vector3f;

namespace xtcore {
    namespace surface {

class Plane: public xtcore::asset::ISurface
{
    public:
	Plane();

	bool intersection(const Ray &ray, hit_record_t* i_hit_record) const;
    NMath::scalar_t distance(NMath::Vector3f p) const;
	void calc_aabb();

    Vector3f point_sample() const;
    Ray ray_sample() const;

	Vector3f normal;
	scalar_t offset;
};

    } /* namespace surface */
} /* namespace xtcore */

#endif /* XTCORE_PLANE_H_INCLUDED */
