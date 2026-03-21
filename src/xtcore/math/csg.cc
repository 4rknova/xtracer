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

inline nmath::scalar_t csg_clamp01(nmath::scalar_t v)
{
    if (v < (nmath::scalar_t)0.0) return (nmath::scalar_t)0.0;
    if (v > (nmath::scalar_t)1.0) return (nmath::scalar_t)1.0;
    return v;
}

inline nmath::scalar_t csg_soft_min(nmath::scalar_t a, nmath::scalar_t b, nmath::scalar_t k)
{
    // Polynomial smooth minimum. k controls blending width.
    if (k <= (nmath::scalar_t)EPSILON) return csg_min(a, b);
    const nmath::scalar_t h = csg_clamp01((nmath::scalar_t)0.5 + (nmath::scalar_t)0.5 * (b - a) / k);
    return ((nmath::scalar_t)1.0 - h) * b + h * a - k * h * ((nmath::scalar_t)1.0 - h);
}

inline nmath::Vector3f estimate_gradient(const xtcore::asset::ISurface *surface, const nmath::Vector3f &p, nmath::scalar_t eps)
{
    if (!surface) return nmath::Vector3f(0.0f, 0.0f, 0.0f);
    const nmath::Vector3f ex(eps, 0.0f, 0.0f);
    const nmath::Vector3f ey(0.0f, eps, 0.0f);
    const nmath::Vector3f ez(0.0f, 0.0f, eps);
    const nmath::scalar_t dx = surface->distance(p + ex) - surface->distance(p - ex);
    const nmath::scalar_t dy = surface->distance(p + ey) - surface->distance(p - ey);
    const nmath::scalar_t dz = surface->distance(p + ez) - surface->distance(p - ez);
    return nmath::Vector3f(dx, dy, dz);
}

inline nmath::Vector3f estimate_csg_gradient(const xtcore::surface::CSG *csg, const nmath::Vector3f &p, nmath::scalar_t eps)
{
    if (!csg) return nmath::Vector3f(0.0f, 0.0f, 0.0f);
    const nmath::Vector3f ex(eps, 0.0f, 0.0f);
    const nmath::Vector3f ey(0.0f, eps, 0.0f);
    const nmath::Vector3f ez(0.0f, 0.0f, eps);
    const nmath::scalar_t dx = csg->distance(p + ex) - csg->distance(p - ex);
    const nmath::scalar_t dy = csg->distance(p + ey) - csg->distance(p - ey);
    const nmath::scalar_t dz = csg->distance(p + ez) - csg->distance(p - ez);
    return nmath::Vector3f(dx, dy, dz);
}

inline bool sample_hit_from_surface(
    const xtcore::asset::ISurface *surface,
    const nmath::Vector3f &hit_point,
    const nmath::Vector3f &surface_normal,
    const Ray &view_ray,
    hit_record_t *out_hit)
{
    if (!surface || !out_hit) return false;

    const nmath::scalar_t probe = (nmath::scalar_t)0.0008;
    const nmath::scalar_t max_point_delta = (nmath::scalar_t)0.006;

    auto try_probe = [&](const Ray &probe_ray) -> bool {
        hit_record_t probe_hit;
        if (!surface->intersection(probe_ray, &probe_hit)) return false;
        if ((probe_hit.point - hit_point).length() > max_point_delta) return false;
        *out_hit = probe_hit;
        return true;
    };

    // First try both sides along the resolved surface normal.
    if (try_probe(Ray(hit_point + surface_normal * probe, -surface_normal))) return true;
    if (try_probe(Ray(hit_point - surface_normal * probe, surface_normal))) return true;

    // Fallback along the camera ray direction in both directions.
    if (try_probe(Ray(hit_point - view_ray.direction * (probe * (nmath::scalar_t)4.0), view_ray.direction))) return true;
    if (try_probe(Ray(hit_point + view_ray.direction * (probe * (nmath::scalar_t)4.0), -view_ray.direction))) return true;

    return false;
}

inline void refine_hit_point_on_surface(
    const xtcore::surface::CSG *csg,
    const Ray &ray,
    nmath::Vector3f *io_point,
    nmath::scalar_t *io_t)
{
    if (!csg || !io_point || !io_t) return;

    const nmath::scalar_t grad_eps = (nmath::scalar_t)0.0001;
    const nmath::scalar_t t_eps = (nmath::scalar_t)EPSILON;
    const int iterations = 6;

    nmath::scalar_t t = *io_t;
    for (int i = 0; i < iterations; ++i) {
        const nmath::Vector3f p = ray.origin + ray.direction * t;
        const nmath::scalar_t sd = csg->distance(p);
        if (std::fabs((double)sd) <= (nmath::scalar_t)1e-5) break;

        const nmath::Vector3f g = estimate_csg_gradient(csg, p, grad_eps);
        const nmath::scalar_t dsd_dt = dot(g, ray.direction);
        if (std::fabs((double)dsd_dt) <= (nmath::scalar_t)1e-7) break;

        const nmath::scalar_t t_next = t - sd / dsd_dt;
        if (!std::isfinite((double)t_next)) break;
        t = std::max(t_eps, t_next);
    }

    *io_point = ray.origin + ray.direction * t;
    *io_t = t;
}

} // namespace

CSG::CSG()
    : op(OP_UNION)
    , smoothness((nmath::scalar_t)0.15)
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
    if (op == OP_SOFT_UNION) return csg_soft_min(dl, dr, smoothness);
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

    const nmath::scalar_t hit_eps = (nmath::scalar_t)0.00001;
    const nmath::scalar_t march_min_step = (nmath::scalar_t)0.00001;
    const nmath::scalar_t max_world_t = (nmath::scalar_t)10000.0;
    const size_t max_steps = 2048;

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
            // Avoid false self-hits when the ray starts near the surface but moves away from it.
            const nmath::Vector3f g = estimate_csg_gradient(this, p, (nmath::scalar_t)0.0001);
            const nmath::scalar_t dsd_dt = dot(g, ray.direction);
            const bool entering_or_tangent = (sd <= (nmath::scalar_t)0.0) || (dsd_dt <= (nmath::scalar_t)0.0);
            if (entering_or_tangent) {
                found = true;
                hit_point = p;
                break;
            }
        }

        t += std::max(ad, march_min_step);
    }

    if (!found || !i_hit_record) return found;

    refine_hit_point_on_surface(this, ray, &hit_point, &t);

    const nmath::scalar_t grad_eps = (nmath::scalar_t)0.0001;
    const nmath::scalar_t dl = left->distance(hit_point);
    const nmath::scalar_t dr = right->distance(hit_point);

    nmath::Vector3f n(0.0f, 0.0f, 0.0f);
    const xtcore::asset::ISurface *prevailing_surface = 0;
    bool flip_prevailing_normal = false;
    if (op == OP_UNION || op == OP_SOFT_UNION) {
        // For union sdf=min(dl,dr), take the prevailing operand's normal.
        if (dl <= dr) {
            prevailing_surface = left;
            n = estimate_gradient(left, hit_point, grad_eps);
        } else {
            prevailing_surface = right;
            n = estimate_gradient(right, hit_point, grad_eps);
        }
    } else if (op == OP_INTERSECTION) {
        // For intersection sdf=max(dl,dr), take the prevailing operand's normal.
        if (dl >= dr) {
            prevailing_surface = left;
            n = estimate_gradient(left, hit_point, grad_eps);
        } else {
            prevailing_surface = right;
            n = estimate_gradient(right, hit_point, grad_eps);
        }
    } else {
        // For difference sdf=max(dl,-dr), right branch contributes with flipped normal.
        if (dl >= -dr) {
            prevailing_surface = left;
            n = estimate_gradient(left, hit_point, grad_eps);
        } else {
            prevailing_surface = right;
            flip_prevailing_normal = true;
            n = -estimate_gradient(right, hit_point, grad_eps);
        }
    }

    // Fallback to composed sdf gradient at branch ties / degeneracies.
    if (n.length() <= (nmath::scalar_t)EPSILON) {
        const nmath::Vector3f ex(grad_eps, 0.0f, 0.0f);
        const nmath::Vector3f ey(0.0f, grad_eps, 0.0f);
        const nmath::Vector3f ez(0.0f, 0.0f, grad_eps);
        const nmath::scalar_t dx = signed_distance(hit_point + ex) - signed_distance(hit_point - ex);
        const nmath::scalar_t dy = signed_distance(hit_point + ey) - signed_distance(hit_point - ey);
        const nmath::scalar_t dz = signed_distance(hit_point + ez) - signed_distance(hit_point - ez);
        n = nmath::Vector3f(dx, dy, dz);
    }
    if (n.length() <= (nmath::scalar_t)EPSILON) {
        n = -ray.direction;
    } else {
        n = n.normalized();
    }
    if (dot(n, ray.direction) > 0.0f) n = -n;

    hit_record_t prevailing_hit;
    const bool have_prevailing_hit = sample_hit_from_surface(prevailing_surface, hit_point, n, ray, &prevailing_hit);
    if (have_prevailing_hit) {
        nmath::Vector3f branch_n = prevailing_hit.normal.normalized();
        if (flip_prevailing_normal) branch_n = -branch_n;
        if (branch_n.length() > (nmath::scalar_t)EPSILON) {
            if (dot(branch_n, ray.direction) > 0.0f) branch_n = -branch_n;
            n = branch_n;
        }
    }

    const nmath::scalar_t csg_surface_bias = (nmath::scalar_t)0.00002;
    const nmath::Vector3f shaded_point = hit_point + n * csg_surface_bias;

    i_hit_record->t = t;
    i_hit_record->point = shaded_point;
    i_hit_record->normal = n;
    i_hit_record->texcoord = nmath::Vector3f(0.0f, 0.0f, 0.0f);
    if (have_prevailing_hit) {
        i_hit_record->texcoord = prevailing_hit.texcoord;
    } else {
        // Fallback: stable normal-based UV so textured CSG does not collapse to (0,0).
        const nmath::scalar_t u = (nmath::scalar_t)(std::atan2((double)n.z, (double)n.x) * 0.15915494309189534 + 0.5);
        const nmath::scalar_t v = (nmath::scalar_t)(n.y * 0.5 + 0.5);
        i_hit_record->texcoord = nmath::Vector3f(u, v, 0.0f);
    }
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

    if (op == OP_UNION || op == OP_SOFT_UNION) {
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
