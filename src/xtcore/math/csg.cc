#include <algorithm>
#include <cmath>
#include <limits>

#include "csg.h"

namespace xtcore {
namespace surface {

namespace {

inline nmath::scalar_t csg_min(nmath::scalar_t a, nmath::scalar_t b)
{
    return (a < b) ? a : b;
}

inline nmath::scalar_t csg_max(nmath::scalar_t a, nmath::scalar_t b)
{
    return (a > b) ? a : b;
}

} // namespace

CSG::CSG()
    : op(OP_UNION)
    , left(0)
    , right(0)
{}

CSG::~CSG()
{
    if (left) {
        delete left;
        left = 0;
    }
    if (right) {
        delete right;
        right = 0;
    }
}

nmath::scalar_t CSG::signed_distance(const nmath::Vector3f &p) const
{
    if (!left || !right) return INFINITY;

    const nmath::scalar_t dl = left->distance(p);
    const nmath::scalar_t dr = right->distance(p);

    if (op == OP_UNION) return csg_min(dl, dr);
    if (op == OP_INTERSECTION) return csg_max(dl, dr);
    return csg_max(dl, -dr); // difference
}

nmath::scalar_t CSG::distance(nmath::Vector3f p) const
{
    return signed_distance(p);
}

bool CSG::is_finite_aabb(const AABB3 &box) const
{
    return std::isfinite((double)box.min.x) && std::isfinite((double)box.min.y) && std::isfinite((double)box.min.z)
        && std::isfinite((double)box.max.x) && std::isfinite((double)box.max.y) && std::isfinite((double)box.max.z);
}

bool CSG::ray_box_interval(const AABB3 &box, const Ray &ray, nmath::scalar_t &tmin, nmath::scalar_t &tmax) const
{
    const nmath::scalar_t eps = (nmath::scalar_t)EPSILON;
    const nmath::scalar_t inf = std::numeric_limits<nmath::scalar_t>::infinity();

    tmin = (nmath::scalar_t)0.0;
    tmax = inf;

    for (int axis = 0; axis < 3; ++axis) {
        const nmath::scalar_t org = ray.origin[axis];
        const nmath::scalar_t dir = ray.direction[axis];
        const nmath::scalar_t bmin = box.min[axis];
        const nmath::scalar_t bmax = box.max[axis];

        if (std::fabs((double)dir) <= eps) {
            if (org < bmin || org > bmax) return false;
            continue;
        }

        nmath::scalar_t t0 = (bmin - org) / dir;
        nmath::scalar_t t1 = (bmax - org) / dir;
        if (t0 > t1) std::swap(t0, t1);

        if (t0 > tmin) tmin = t0;
        if (t1 < tmax) tmax = t1;
        if (tmax < tmin) return false;
    }

    if (tmax < (nmath::scalar_t)0.0) return false;
    if (tmin < (nmath::scalar_t)0.0) tmin = (nmath::scalar_t)0.0;
    return true;
}

bool CSG::intersection(const Ray &ray, hit_record_t *i_hit_record) const
{
    if (!left || !right) return false;

    const nmath::scalar_t hit_eps = (nmath::scalar_t)0.0008;
    const nmath::scalar_t march_min_step = (nmath::scalar_t)0.0005;
    const nmath::scalar_t max_world_t = (nmath::scalar_t)10000.0;
    const size_t max_steps = 640;

    nmath::scalar_t tmin = (nmath::scalar_t)0.0;
    nmath::scalar_t tmax = max_world_t;
    if (is_finite_aabb(aabb)) {
        if (!ray_box_interval(aabb, ray, tmin, tmax)) return false;
        tmax = std::min(tmax, max_world_t);
    }

    nmath::scalar_t t = tmin + (nmath::scalar_t)EPSILON;
    bool found = false;
    nmath::Vector3f hit_point(0.0f, 0.0f, 0.0f);

    for (size_t i = 0; i < max_steps && t <= tmax; ++i) {
        const nmath::Vector3f p = ray.origin + ray.direction * t;
        const nmath::scalar_t sd = signed_distance(p);
        const nmath::scalar_t ad = std::fabs((double)sd);

        if (ad <= hit_eps) {
            found = true;
            hit_point = p;
            break;
        }

        t += std::max(ad, march_min_step);
    }

    if (!found || !i_hit_record) return found;

    const nmath::scalar_t grad_eps = (nmath::scalar_t)0.0005;
    const nmath::Vector3f ex(grad_eps, 0.0f, 0.0f);
    const nmath::Vector3f ey(0.0f, grad_eps, 0.0f);
    const nmath::Vector3f ez(0.0f, 0.0f, grad_eps);

    const nmath::scalar_t dx = signed_distance(hit_point + ex) - signed_distance(hit_point - ex);
    const nmath::scalar_t dy = signed_distance(hit_point + ey) - signed_distance(hit_point - ey);
    const nmath::scalar_t dz = signed_distance(hit_point + ez) - signed_distance(hit_point - ez);

    nmath::Vector3f n(dx, dy, dz);
    if (n.length() <= (nmath::scalar_t)EPSILON) {
        n = -ray.direction;
    } else {
        n = n.normalized();
    }
    if (dot(n, ray.direction) > 0.0f) n = -n;

    i_hit_record->t = t;
    i_hit_record->point = hit_point;
    i_hit_record->normal = n;
    i_hit_record->texcoord = nmath::Vector3f(0.0f, 0.0f, 0.0f);
    i_hit_record->incident_direction = ray.direction;

    return true;
}

void CSG::calc_aabb()
{
    if (!left || !right) {
        aabb.min = nmath::Vector3f(-INFINITY, -INFINITY, -INFINITY);
        aabb.max = nmath::Vector3f(INFINITY, INFINITY, INFINITY);
        return;
    }

    left->calc_aabb();
    right->calc_aabb();

    const AABB3 &lb = left->aabb;
    const AABB3 &rb = right->aabb;

    if (op == OP_UNION) {
        aabb.min = nmath::Vector3f(
            std::min(lb.min.x, rb.min.x),
            std::min(lb.min.y, rb.min.y),
            std::min(lb.min.z, rb.min.z)
        );
        aabb.max = nmath::Vector3f(
            std::max(lb.max.x, rb.max.x),
            std::max(lb.max.y, rb.max.y),
            std::max(lb.max.z, rb.max.z)
        );
        return;
    }

    if (op == OP_INTERSECTION) {
        aabb.min = nmath::Vector3f(
            std::max(lb.min.x, rb.min.x),
            std::max(lb.min.y, rb.min.y),
            std::max(lb.min.z, rb.min.z)
        );
        aabb.max = nmath::Vector3f(
            std::min(lb.max.x, rb.max.x),
            std::min(lb.max.y, rb.max.y),
            std::min(lb.max.z, rb.max.z)
        );
        if (aabb.min.x > aabb.max.x || aabb.min.y > aabb.max.y || aabb.min.z > aabb.max.z) {
            const nmath::Vector3f c = (aabb.min + aabb.max) * 0.5f;
            aabb.min = c;
            aabb.max = c;
        }
        return;
    }

    // Difference cannot extend outside left primitive bounds.
    aabb = lb;
}

Vector3f CSG::point_sample() const
{
    if (is_finite_aabb(aabb)) return aabb.center();
    if (left) return left->point_sample();
    return Vector3f(0, 0, 0);
}

Ray CSG::ray_sample() const
{
    Ray ray;
    ray.origin = point_sample();
    return ray;
}

Vector3f CSG::emitter_position() const
{
    if (is_finite_aabb(aabb)) return aabb.center();
    if (left) return left->emitter_position();
    return Vector3f(0, 0, 0);
}

} /* namespace surface */
} /* namespace xtcore */
