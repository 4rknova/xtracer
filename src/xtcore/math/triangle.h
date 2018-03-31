#ifndef XTCORE_TRIANGLE_H_INCLUDED
#define XTCORE_TRIANGLE_H_INCLUDED

#include <nmath/defs.h>
#include <nmath/precision.h>
#include <nmath/types.h>
#include <nmath/vector.h>
#include "surface.h"
#include "ray.h"

namespace xtcore {
    namespace surface {

class Triangle: public xtcore::asset::ISurface
{
    public:
	bool intersection(const Ray &ray, HitRecord* i_info) const;
	void calc_aabb();
	Vector3f calc_normal() const;
	Vector3f calc_barycentric(const Vector3f &p) const;

    Vector3f point_sample() const;
    Ray ray_sample() const;

    Vector3f  v[3]; // position
    Vector3f  n[3]; // normal
    Vector2f tc[3]; // texcoords
};

    } /* namespace surface */
} /* namespace xtcore */

#endif /* XTCORE_TRIANGLE_H_INCLUDED */
