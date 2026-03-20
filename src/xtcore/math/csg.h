#ifndef XTCORE_CSG_H_INCLUDED
#define XTCORE_CSG_H_INCLUDED

#include "surface.h"

namespace xtcore {
namespace surface {

class CSG : public xtcore::asset::ISurface
{
public:
    enum op_t {
        OP_UNION = 0,
        OP_INTERSECTION,
        OP_DIFFERENCE
    };

    CSG();
    ~CSG();

    bool intersection(const Ray &ray, hit_record_t *i_hit_record) const;
    nmath::scalar_t distance(nmath::Vector3f p) const;
    void calc_aabb();

    Vector3f point_sample() const;
    Ray ray_sample() const;
    Vector3f emitter_position() const;

    op_t op;
    xtcore::asset::ISurface *left;
    xtcore::asset::ISurface *right;

private:
    nmath::scalar_t signed_distance(const nmath::Vector3f &p) const;
    bool is_finite_aabb(const AABB3 &box) const;
    bool ray_box_interval(const AABB3 &box, const Ray &ray, nmath::scalar_t &tmin, nmath::scalar_t &tmax) const;
};

} /* namespace surface */
} /* namespace xtcore */

#endif /* XTCORE_CSG_H_INCLUDED */
