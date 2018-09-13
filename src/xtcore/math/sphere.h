#ifndef XTCORE_SPHERE_H_INCLUDED
#define XTCORE_SPHERE_H_INCLUDED

#include <nmath/defs.h>
#include <nmath/precision.h>
#include <nmath/vector.h>
#include "surface.h"
#include "ray.h"

using NMath::Vector3f;

namespace xtcore {
    namespace surface {

#define XTCORE_SPHERE_DEFAULT_RADIUS 1.0

class Sphere: public xtcore::asset::ISurface
{
    public:
        Sphere();
        Sphere(const Vector3f &org, scalar_t rad);

		bool intersection(const Ray &ray, hit_record_t* i_hit_record) const;
        NMath::scalar_t distance(NMath::Vector3f p) const;
		void calc_aabb();

        Vector3f point_sample() const;
        Ray ray_sample() const;

        Vector3f origin;
        scalar_t radius;
};

    } /* namespace surface */
} /* namespace xtcore */

#endif /* XTCORE_SPHERE_H_INCLUDED */
