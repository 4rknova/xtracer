#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include <nmath/precision.h>
#include <nmath/vector.h>

#include "fractal.h"
#include "hitrecord.h"

namespace xtcore {
namespace surface {

namespace {

typedef std::array<Vector3f, 4> tetra_t;

static scalar_t axis_get(const Vector3f &v, int axis)
{
    if (axis == 0) return v.x;
    if (axis == 1) return v.y;
    return v.z;
}

static Vector3f axis_normal(int axis, scalar_t sign)
{
    if (axis == 0) return Vector3f(sign, 0.0f, 0.0f);
    if (axis == 1) return Vector3f(0.0f, sign, 0.0f);
    return Vector3f(0.0f, 0.0f, sign);
}

static Vector3f vec_abs(const Vector3f &v)
{
    return Vector3f(nmath_abs(v.x), nmath_abs(v.y), nmath_abs(v.z));
}

static scalar_t clamp_unit(scalar_t v)
{
    return std::max((scalar_t)0.0, std::min((scalar_t)1.0, v));
}

static scalar_t clamp_signed_unit(scalar_t v)
{
    return std::max((scalar_t)-1.0, std::min((scalar_t)1.0, v));
}

static scalar_t wrap_unit(scalar_t v)
{
    while (v > 1.0f) v -= 2.0f;
    while (v < -1.0f) v += 2.0f;
    return v;
}

static Vector3f rotate_x(const Vector3f &v, scalar_t a)
{
    const scalar_t c = nmath_cos(a);
    const scalar_t s = nmath_sin(a);
    return Vector3f(v.x, v.y * c - v.z * s, v.y * s + v.z * c);
}

static Vector3f rotate_y(const Vector3f &v, scalar_t a)
{
    const scalar_t c = nmath_cos(a);
    const scalar_t s = nmath_sin(a);
    return Vector3f(v.x * c + v.z * s, v.y, -v.x * s + v.z * c);
}

static Vector3f rotate_z(const Vector3f &v, scalar_t a)
{
    const scalar_t c = nmath_cos(a);
    const scalar_t s = nmath_sin(a);
    return Vector3f(v.x * c - v.y * s, v.x * s + v.y * c, v.z);
}

static Vector3f rotate_euler_xyz(const Vector3f &v, const Vector3f &euler)
{
    const scalar_t ax = euler.x;
    const scalar_t ay = euler.y;
    const scalar_t az = euler.z;
    return rotate_z(rotate_y(rotate_x(v, ax), ay), az);
}

static Vector3f inverse_rotate_euler_xyz(const Vector3f &v, const Vector3f &euler)
{
    const scalar_t ax = -euler.x;
    const scalar_t ay = -euler.y;
    const scalar_t az = -euler.z;
    return rotate_x(rotate_y(rotate_z(v, az), ay), ax);
}

static bool ray_aabb_hit(const Ray &ray, const Vector3f &bmin, const Vector3f &bmax,
                         scalar_t *o_t_near = nullptr, scalar_t *o_t_far = nullptr)
{
    scalar_t t_near = -INFINITY;
    scalar_t t_far = INFINITY;

    for (int axis = 0; axis < 3; ++axis) {
        const scalar_t o = axis_get(ray.origin, axis);
        const scalar_t d = axis_get(ray.direction, axis);
        const scalar_t lo = axis_get(bmin, axis);
        const scalar_t hi = axis_get(bmax, axis);

        if (nmath_abs(d) <= EPSILON) {
            if (o < lo || o > hi) return false;
            continue;
        }

        scalar_t t1 = (lo - o) / d;
        scalar_t t2 = (hi - o) / d;
        if (t1 > t2) std::swap(t1, t2);

        if (t1 > t_near) t_near = t1;
        if (t2 < t_far) t_far = t2;
        if (t_far < t_near) return false;
    }

    if (o_t_near) *o_t_near = t_near;
    if (o_t_far) *o_t_far = t_far;
    return true;
}

static bool ray_aabb_hit_with_normal(const Ray &ray, const Vector3f &bmin, const Vector3f &bmax,
                                     scalar_t &o_t, Vector3f &o_n)
{
    scalar_t t_near = -INFINITY;
    scalar_t t_far = INFINITY;
    int near_axis = 2;
    int far_axis = 2;
    scalar_t near_sign = 1.0f;
    scalar_t far_sign = -1.0f;

    for (int axis = 0; axis < 3; ++axis) {
        const scalar_t o = axis_get(ray.origin, axis);
        const scalar_t d = axis_get(ray.direction, axis);
        const scalar_t lo = axis_get(bmin, axis);
        const scalar_t hi = axis_get(bmax, axis);

        if (nmath_abs(d) <= EPSILON) {
            if (o < lo || o > hi) return false;
            continue;
        }

        scalar_t t1 = (lo - o) / d;
        scalar_t t2 = (hi - o) / d;
        scalar_t nsign = (d >= 0.0f) ? -1.0f : 1.0f;

        if (t1 > t2) {
            std::swap(t1, t2);
            nsign = -nsign;
        }

        if (t1 > t_near) {
            t_near = t1;
            near_axis = axis;
            near_sign = nsign;
        }
        if (t2 < t_far) {
            t_far = t2;
            far_axis = axis;
            far_sign = -nsign;
        }
        if (t_far < t_near) return false;
    }

    if (t_far < EPSILON) return false;

    if (t_near > EPSILON) {
        o_t = t_near;
        o_n = axis_normal(near_axis, near_sign);
    } else {
        o_t = t_far;
        o_n = axis_normal(far_axis, far_sign);
    }
    return true;
}

static bool inside_menger_unit(const Vector3f &p, size_t iterations)
{
    if (nmath_abs(p.x) > 1.0f || nmath_abs(p.y) > 1.0f || nmath_abs(p.z) > 1.0f) return false;
    Vector3f q = p;
    const int depth = (int)std::max((size_t)1, iterations);
    for (int i = 0; i < depth; ++i) {
        const Vector3f a = vec_abs(q);
        const bool cut_xy = (a.x < (1.0f / 3.0f) && a.y < (1.0f / 3.0f));
        const bool cut_xz = (a.x < (1.0f / 3.0f) && a.z < (1.0f / 3.0f));
        const bool cut_yz = (a.y < (1.0f / 3.0f) && a.z < (1.0f / 3.0f));
        if (cut_xy || cut_xz || cut_yz) return false;

        q.x = wrap_unit(q.x * 3.0f);
        q.y = wrap_unit(q.y * 3.0f);
        q.z = wrap_unit(q.z * 3.0f);
    }
    return true;
}

static bool tetra_barycentric(const Vector3f &p, const tetra_t &v, scalar_t (&b)[4])
{
    const Vector3f e1 = v[1] - v[0];
    const Vector3f e2 = v[2] - v[0];
    const Vector3f e3 = v[3] - v[0];
    const Vector3f r = p - v[0];

    const Vector3f c23 = nmath::cross(e2, e3);
    const Vector3f cr3 = nmath::cross(r, e3);
    const Vector3f c2r = nmath::cross(e2, r);
    const scalar_t det = nmath::dot(e1, c23);
    if (nmath_abs(det) <= EPSILON) return false;

    const scalar_t inv_det = 1.0f / det;
    const scalar_t b1 = nmath::dot(r, c23) * inv_det;
    const scalar_t b2 = nmath::dot(e1, cr3) * inv_det;
    const scalar_t b3 = nmath::dot(e1, c2r) * inv_det;

    b[0] = 1.0f - b1 - b2 - b3;
    b[1] = b1;
    b[2] = b2;
    b[3] = b3;
    return true;
}

static bool inside_sierpinski_unit(const Vector3f &p, size_t iterations)
{
    const scalar_t s = 1.0f / nmath_sqrt(3.0f);
    tetra_t base = {
        Vector3f( s,  s,  s),
        Vector3f(-s, -s,  s),
        Vector3f(-s,  s, -s),
        Vector3f( s, -s, -s)
    };

    scalar_t b[4];
    if (!tetra_barycentric(p, base, b)) return false;

    const scalar_t eps = 1e-6f;
    for (int i = 0; i < 4; ++i) {
        if (b[i] < -eps || b[i] > 1.0f + eps) return false;
    }

    const int depth = (int)std::max((size_t)1, iterations);
    for (int iter = 0; iter < depth - 1; ++iter) {
        int pivot = 0;
        for (int i = 1; i < 4; ++i) if (b[i] > b[pivot]) pivot = i;
        if (b[pivot] < 0.5f - eps) return false;

        scalar_t next[4];
        for (int i = 0; i < 4; ++i) {
            next[i] = (i == pivot) ? (2.0f * b[i] - 1.0f) : (2.0f * b[i]);
        }
        for (int i = 0; i < 4; ++i) {
            b[i] = next[i];
            if (b[i] < -1e-4f || b[i] > 1.0f + 1e-4f) return false;
        }
    }
    return true;
}

static void tetra_aabb(const tetra_t &t, Vector3f &bmin, Vector3f &bmax)
{
    bmin = Vector3f(INFINITY, INFINITY, INFINITY);
    bmax = Vector3f(-INFINITY, -INFINITY, -INFINITY);
    for (int i = 0; i < 4; ++i) {
        bmin.x = std::min(bmin.x, t[i].x);
        bmin.y = std::min(bmin.y, t[i].y);
        bmin.z = std::min(bmin.z, t[i].z);
        bmax.x = std::max(bmax.x, t[i].x);
        bmax.y = std::max(bmax.y, t[i].y);
        bmax.z = std::max(bmax.z, t[i].z);
    }
}

static bool ray_tetra_hit(const Ray &ray, const tetra_t &tet, scalar_t &o_t, Vector3f &o_n)
{
    scalar_t t_enter = -INFINITY;
    scalar_t t_exit = INFINITY;
    Vector3f n_enter(0.0f, 1.0f, 0.0f);
    Vector3f n_exit(0.0f, -1.0f, 0.0f);

    const int faces[4][3] = {
        {1, 2, 3},
        {0, 3, 2},
        {0, 1, 3},
        {0, 2, 1}
    };

    for (int i = 0; i < 4; ++i) {
        const Vector3f &a = tet[faces[i][0]];
        const Vector3f &b = tet[faces[i][1]];
        const Vector3f &c = tet[faces[i][2]];
        const Vector3f &opp = tet[i];

        Vector3f n = nmath::cross(b - a, c - a);
        if (n.length() <= EPSILON) return false;
        n.normalize();

        if (nmath::dot(n, opp - a) > 0.0f) n = -n;

        const scalar_t dist = nmath::dot(n, ray.origin - a);
        const scalar_t dn = nmath::dot(n, ray.direction);

        if (nmath_abs(dn) <= EPSILON) {
            if (dist > 0.0f) return false;
            continue;
        }

        const scalar_t t = -dist / dn;
        if (dn < 0.0f) {
            if (t > t_enter) {
                t_enter = t;
                n_enter = n;
            }
        } else {
            if (t < t_exit) {
                t_exit = t;
                n_exit = n;
            }
        }

        if (t_enter > t_exit) return false;
    }

    if (t_exit < EPSILON) return false;
    if (t_enter > EPSILON) {
        o_t = t_enter;
        o_n = n_enter;
    } else {
        o_t = t_exit;
        o_n = n_exit;
    }
    return true;
}

struct fractal_hit_t {
    bool hit;
    scalar_t t;
    Vector3f normal;
    fractal_hit_t() : hit(false), t(INFINITY), normal(0.0f, 1.0f, 0.0f) {}
};

static void intersect_menger_recursive(const Ray &ray,
                                       const Vector3f &center,
                                       scalar_t half_extent,
                                       size_t depth,
                                       fractal_hit_t &best)
{
    const Vector3f bmin = center - Vector3f(half_extent, half_extent, half_extent);
    const Vector3f bmax = center + Vector3f(half_extent, half_extent, half_extent);
    scalar_t t_near = 0.0f;
    scalar_t t_far = 0.0f;
    if (!ray_aabb_hit(ray, bmin, bmax, &t_near, &t_far)) return;
    if (t_near > best.t) return;

    if (depth == 0) {
        scalar_t t = 0.0f;
        Vector3f n;
        if (ray_aabb_hit_with_normal(ray, bmin, bmax, t, n) && t < best.t) {
            best.hit = true;
            best.t = t;
            best.normal = n;
        }
        return;
    }

    const scalar_t child_half = half_extent / 3.0f;
    for (int ix = -1; ix <= 1; ++ix) {
        for (int iy = -1; iy <= 1; ++iy) {
            for (int iz = -1; iz <= 1; ++iz) {
                const int zeros = (ix == 0) + (iy == 0) + (iz == 0);
                if (zeros >= 2) continue;
                const Vector3f child_center = center + Vector3f((scalar_t)ix, (scalar_t)iy, (scalar_t)iz) * (child_half * 2.0f);
                intersect_menger_recursive(ray, child_center, child_half, depth - 1, best);
            }
        }
    }
}

static void sierpinski_children(const tetra_t &parent, tetra_t (&children)[4])
{
    const Vector3f m01 = (parent[0] + parent[1]) * 0.5f;
    const Vector3f m02 = (parent[0] + parent[2]) * 0.5f;
    const Vector3f m03 = (parent[0] + parent[3]) * 0.5f;
    const Vector3f m12 = (parent[1] + parent[2]) * 0.5f;
    const Vector3f m13 = (parent[1] + parent[3]) * 0.5f;
    const Vector3f m23 = (parent[2] + parent[3]) * 0.5f;

    children[0] = {parent[0], m01, m02, m03};
    children[1] = {m01, parent[1], m12, m13};
    children[2] = {m02, m12, parent[2], m23};
    children[3] = {m03, m13, m23, parent[3]};
}

static void intersect_sierpinski_recursive(const Ray &ray, const tetra_t &tet, size_t depth, fractal_hit_t &best)
{
    Vector3f bmin, bmax;
    tetra_aabb(tet, bmin, bmax);
    scalar_t t_near = 0.0f;
    scalar_t t_far = 0.0f;
    if (!ray_aabb_hit(ray, bmin, bmax, &t_near, &t_far)) return;
    if (t_near > best.t) return;

    if (depth <= 1) {
        scalar_t t = 0.0f;
        Vector3f n;
        if (ray_tetra_hit(ray, tet, t, n) && t < best.t) {
            best.hit = true;
            best.t = t;
            best.normal = n;
        }
        return;
    }

    tetra_t children[4];
    sierpinski_children(tet, children);
    for (int i = 0; i < 4; ++i) {
        intersect_sierpinski_recursive(ray, children[i], depth - 1, best);
    }
}

static scalar_t sd_box_unit(const Vector3f &p)
{
    const Vector3f q = vec_abs(p) - Vector3f(1.0f, 1.0f, 1.0f);
    const Vector3f mq(std::max((scalar_t)0.0, q.x), std::max((scalar_t)0.0, q.y), std::max((scalar_t)0.0, q.z));
    const scalar_t out = mq.length();
    const scalar_t in = std::min(std::max(q.x, std::max(q.y, q.z)), (scalar_t)0.0);
    return out + in;
}

static scalar_t point_segment_distance(const Vector3f &p, const Vector3f &a, const Vector3f &b)
{
    const Vector3f ab = b - a;
    const scalar_t den = nmath::dot(ab, ab);
    if (den <= EPSILON) return (p - a).length();
    const scalar_t t = clamp_unit(nmath::dot(p - a, ab) / den);
    return (p - (a + ab * t)).length();
}

static scalar_t point_triangle_distance(const Vector3f &p, const Vector3f &a, const Vector3f &b, const Vector3f &c)
{
    const Vector3f ab = b - a;
    const Vector3f ac = c - a;
    const Vector3f ap = p - a;
    const scalar_t d1 = nmath::dot(ab, ap);
    const scalar_t d2 = nmath::dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) return (p - a).length();

    const Vector3f bp = p - b;
    const scalar_t d3 = nmath::dot(ab, bp);
    const scalar_t d4 = nmath::dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) return (p - b).length();

    const scalar_t vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        const scalar_t v = d1 / (d1 - d3);
        return (p - (a + ab * v)).length();
    }

    const Vector3f cp = p - c;
    const scalar_t d5 = nmath::dot(ab, cp);
    const scalar_t d6 = nmath::dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) return (p - c).length();

    const scalar_t vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        const scalar_t w = d2 / (d2 - d6);
        return (p - (a + ac * w)).length();
    }

    const scalar_t va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        const Vector3f bc = c - b;
        const scalar_t w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return (p - (b + bc * w)).length();
    }

    Vector3f n = nmath::cross(ab, ac);
    if (n.length() <= EPSILON) {
        const scalar_t da = point_segment_distance(p, a, b);
        const scalar_t db = point_segment_distance(p, b, c);
        const scalar_t dc = point_segment_distance(p, c, a);
        return std::min(da, std::min(db, dc));
    }
    n.normalize();
    return nmath_abs(nmath::dot(p - a, n));
}

static scalar_t tetra_surface_distance(const Vector3f &p, const tetra_t &tet)
{
    const scalar_t d0 = point_triangle_distance(p, tet[1], tet[2], tet[3]);
    const scalar_t d1 = point_triangle_distance(p, tet[0], tet[3], tet[2]);
    const scalar_t d2 = point_triangle_distance(p, tet[0], tet[1], tet[3]);
    const scalar_t d3 = point_triangle_distance(p, tet[0], tet[2], tet[1]);
    return std::min(std::min(d0, d1), std::min(d2, d3));
}

static scalar_t fractal_power_distance_estimator(const Vector3f &p,
                                                 const Vector3f &c,
                                                 size_t iterations,
                                                 scalar_t power,
                                                 scalar_t bailout,
                                                 bool use_seed_c)
{
    Vector3f z = p;
    scalar_t dr = 1.0f;
    scalar_t r = 0.0f;

    const size_t iters = std::max((size_t)1, iterations);
    const scalar_t pw = std::max((scalar_t)2.0, power);
    const scalar_t bo = std::max((scalar_t)2.0, bailout);

    for (size_t i = 0; i < iters; ++i) {
        r = z.length();
        if (r > bo) break;
        if (r < 1e-9f) {
            z = c;
            continue;
        }

        const scalar_t theta = (scalar_t)std::acos((double)clamp_signed_unit(z.z / r));
        const scalar_t phi = (scalar_t)std::atan2((double)z.y, (double)z.x);
        const scalar_t zr_pw_1 = (scalar_t)std::pow((double)r, (double)(pw - 1.0f));
        dr = zr_pw_1 * pw * dr + 1.0f;

        const scalar_t zr = zr_pw_1 * r;
        const scalar_t t = theta * pw;
        const scalar_t ph = phi * pw;
        const scalar_t st = (scalar_t)std::sin((double)t);
        const Vector3f zn(
            zr * st * (scalar_t)std::cos((double)ph),
            zr * st * (scalar_t)std::sin((double)ph),
            zr * (scalar_t)std::cos((double)t)
        );
        z = zn + (use_seed_c ? c : p);
    }

    r = z.length();
    if (!std::isfinite((double)r) || !std::isfinite((double)dr) || dr <= EPSILON) return 1e6f;
    if (r <= EPSILON) return 0.0f;
    return (scalar_t)(0.5 * std::log((double)r) * (double)r / (double)dr);
}

static Vector3f estimate_distance_normal(const xtcore::asset::ISurface &s, const Vector3f &p, scalar_t h)
{
    // Fractal DEs are approximate and can become sign-unstable near the surface.
    // Sample the unsigned field (same as raymarch stepping) with tetrahedral
    // offsets to reduce cancellation artifacts.
    const scalar_t eps = std::max((scalar_t)1e-4, h);
    const Vector3f e1( 1.0f, -1.0f, -1.0f);
    const Vector3f e2(-1.0f, -1.0f,  1.0f);
    const Vector3f e3(-1.0f,  1.0f, -1.0f);
    const Vector3f e4( 1.0f,  1.0f,  1.0f);

    const scalar_t d1 = nmath_abs(s.distance(p + e1 * eps));
    const scalar_t d2 = nmath_abs(s.distance(p + e2 * eps));
    const scalar_t d3 = nmath_abs(s.distance(p + e3 * eps));
    const scalar_t d4 = nmath_abs(s.distance(p + e4 * eps));

    Vector3f n = e1 * d1 + e2 * d2 + e3 * d3 + e4 * d4;
    if (n.length() <= EPSILON) return Vector3f(0.0f, 1.0f, 0.0f);
    n.normalize();
    return n;
}

static bool intersect_raymarch(const xtcore::asset::ISurface &surface,
                               const Ray &ray,
                               scalar_t radius,
                               scalar_t hit_epsilon,
                               int max_steps,
                               hit_record_t *i_hit_record)
{
    scalar_t t_near = 0.0f;
    scalar_t t_far = 0.0f;
    if (!surface.aabb.intersection(ray)) return false;
    if (!ray_aabb_hit(ray, surface.aabb.min, surface.aabb.max, &t_near, &t_far)) return false;

    const scalar_t ray_bias = std::max((scalar_t)EPSILON, radius * (scalar_t)0.0010);
    scalar_t t = std::max(ray_bias, t_near);
    const scalar_t t_end = t_far + radius * 0.5f;
    const scalar_t min_step = std::max((scalar_t)1e-4, radius * (scalar_t)0.0005);
    const scalar_t eps_hit = std::max((scalar_t)1e-4, radius * hit_epsilon);

    for (int step = 0; step < max_steps && t <= t_end; ++step) {
        const Vector3f p = ray.origin + ray.direction * t;
        scalar_t d = surface.distance(p);
        if (!std::isfinite((double)d)) return false;
        d = nmath_abs(d);

        if (d <= eps_hit) {
            if (i_hit_record) {
                Vector3f n = estimate_distance_normal(surface, p, radius * (scalar_t)0.0030);
                if (nmath::dot(n, ray.direction) > 0.0f) n = -n;
                const Vector3f shaded_point = p + n * (eps_hit * (scalar_t)2.0);
                i_hit_record->t = t;
                i_hit_record->point = shaded_point;
                i_hit_record->normal = n;
                const Vector3f lp = (shaded_point - surface.aabb.min);
                const Vector3f ext = surface.aabb.max - surface.aabb.min;
                const scalar_t ux = (ext.x > EPSILON) ? (lp.x / ext.x) : 0.5f;
                const scalar_t uy = (ext.y > EPSILON) ? (lp.y / ext.y) : 0.5f;
                i_hit_record->texcoord = Vector3f(ux, uy, 0.0f);
                i_hit_record->incident_direction = ray.direction;
            }
            return true;
        }

        t += std::max(min_step, d * (scalar_t)0.8);
    }
    return false;
}

} /* namespace */

MengerSponge::MengerSponge()
    : origin(0.0f, 0.0f, 0.0f)
    , orientation(0.0f, 0.0f, 0.0f)
    , radius(1.0f)
    , iterations(2)
{}

bool MengerSponge::intersection(const Ray &ray, hit_record_t *i_hit_record) const
{
    Ray local_ray;
    local_ray.origin = inverse_rotate_euler_xyz(ray.origin - origin, orientation);
    local_ray.direction = inverse_rotate_euler_xyz(ray.direction, orientation).normalized();

    fractal_hit_t hit;
    const size_t depth = std::max((size_t)1, std::min((size_t)5, iterations));
    intersect_menger_recursive(local_ray, Vector3f(0.0f, 0.0f, 0.0f), std::max((scalar_t)EPSILON, radius), depth, hit);
    if (!hit.hit || hit.t <= EPSILON) return false;

    if (i_hit_record) {
        i_hit_record->t = hit.t;
        const Vector3f local_point = local_ray.origin + local_ray.direction * hit.t;
        const Vector3f world_point = rotate_euler_xyz(local_point, orientation) + origin;
        Vector3f world_normal = rotate_euler_xyz(hit.normal, orientation).normalized();
        if (nmath::dot(world_normal, ray.direction) > 0.0f) world_normal = -world_normal;
        i_hit_record->point = world_point;
        i_hit_record->normal = world_normal;
        const Vector3f lp = local_point / std::max((scalar_t)EPSILON, radius);
        i_hit_record->texcoord = Vector3f(lp.x * 0.5f + 0.5f, lp.y * 0.5f + 0.5f, 0.0f);
        i_hit_record->incident_direction = ray.direction;
    }
    return true;
}

nmath::scalar_t MengerSponge::distance(nmath::Vector3f p) const
{
    const scalar_t r = std::max((scalar_t)EPSILON, radius);
    const Vector3f lp = inverse_rotate_euler_xyz(p - origin, orientation) / r;
    if (inside_menger_unit(lp, iterations)) return -EPSILON;
    return sd_box_unit(lp) * r;
}

void MengerSponge::calc_aabb()
{
    const scalar_t r = std::max((scalar_t)EPSILON, radius);
    const Vector3f corners[8] = {
        Vector3f(-r, -r, -r), Vector3f(-r, -r,  r),
        Vector3f(-r,  r, -r), Vector3f(-r,  r,  r),
        Vector3f( r, -r, -r), Vector3f( r, -r,  r),
        Vector3f( r,  r, -r), Vector3f( r,  r,  r)
    };

    Vector3f bmin( INFINITY,  INFINITY,  INFINITY);
    Vector3f bmax(-INFINITY, -INFINITY, -INFINITY);
    for (size_t i = 0; i < 8; ++i) {
        const Vector3f p = rotate_euler_xyz(corners[i], orientation) + origin;
        if (p.x < bmin.x) bmin.x = p.x;
        if (p.y < bmin.y) bmin.y = p.y;
        if (p.z < bmin.z) bmin.z = p.z;
        if (p.x > bmax.x) bmax.x = p.x;
        if (p.y > bmax.y) bmax.y = p.y;
        if (p.z > bmax.z) bmax.z = p.z;
    }
    aabb.min = bmin;
    aabb.max = bmax;
}

Vector3f MengerSponge::point_sample() const
{
    return origin;
}

Ray MengerSponge::ray_sample() const
{
    Ray ray;
    ray.origin = origin;
    ray.direction = Vector3f(0.0f, 1.0f, 0.0f);
    return ray;
}

Vector3f MengerSponge::emitter_position() const
{
    return origin;
}

SierpinskiTetrahedron::SierpinskiTetrahedron()
    : origin(0.0f, 0.0f, 0.0f)
    , radius(1.0f)
    , iterations(2)
{}

bool SierpinskiTetrahedron::intersection(const Ray &ray, hit_record_t *i_hit_record) const
{
    const scalar_t r = std::max((scalar_t)EPSILON, radius);
    const scalar_t s = 1.0f / nmath_sqrt(3.0f);
    const tetra_t root = {
        origin + Vector3f( s,  s,  s) * r,
        origin + Vector3f(-s, -s,  s) * r,
        origin + Vector3f(-s,  s, -s) * r,
        origin + Vector3f( s, -s, -s) * r
    };

    fractal_hit_t hit;
    const size_t depth = std::max((size_t)1, std::min((size_t)8, iterations));
    intersect_sierpinski_recursive(ray, root, depth, hit);
    if (!hit.hit || hit.t <= EPSILON) return false;

    if (i_hit_record) {
        i_hit_record->t = hit.t;
        i_hit_record->point = ray.origin + ray.direction * hit.t;
        i_hit_record->normal = hit.normal;
        const Vector3f lp = (i_hit_record->point - origin) / (r * s);
        i_hit_record->texcoord = Vector3f(lp.x * 0.5f + 0.5f, lp.y * 0.5f + 0.5f, 0.0f);
        i_hit_record->incident_direction = ray.direction;
    }
    return true;
}

nmath::scalar_t SierpinskiTetrahedron::distance(nmath::Vector3f p) const
{
    const scalar_t r = std::max((scalar_t)EPSILON, radius);
    if (inside_sierpinski_unit((p - origin) / r, iterations)) return -EPSILON;

    const scalar_t s = 1.0f / nmath_sqrt(3.0f);
    const tetra_t root = {
        origin + Vector3f( s,  s,  s) * r,
        origin + Vector3f(-s, -s,  s) * r,
        origin + Vector3f(-s,  s, -s) * r,
        origin + Vector3f( s, -s, -s) * r
    };
    return tetra_surface_distance(p, root);
}

void SierpinskiTetrahedron::calc_aabb()
{
    const scalar_t r = std::max((scalar_t)EPSILON, radius);
    const scalar_t s = 1.0f / nmath_sqrt(3.0f);
    const scalar_t e = r * s;
    aabb.min = origin - Vector3f(e, e, e);
    aabb.max = origin + Vector3f(e, e, e);
}

Vector3f SierpinskiTetrahedron::point_sample() const
{
    return origin;
}

Ray SierpinskiTetrahedron::ray_sample() const
{
    Ray ray;
    ray.origin = origin;
    ray.direction = Vector3f(0.0f, 1.0f, 0.0f);
    return ray;
}

Vector3f SierpinskiTetrahedron::emitter_position() const
{
    return origin;
}

Mandelbulb::Mandelbulb()
    : origin(0.0f, 0.0f, 0.0f)
    , radius(1.0f)
    , iterations(18)
    , power(8.0f)
    , bailout(4.0f)
{}

bool Mandelbulb::intersection(const Ray &ray, hit_record_t *i_hit_record) const
{
    return intersect_raymarch(*this, ray, std::max((scalar_t)EPSILON, radius), (scalar_t)0.0012, 220, i_hit_record);
}

nmath::scalar_t Mandelbulb::distance(nmath::Vector3f p) const
{
    const scalar_t r = std::max((scalar_t)EPSILON, radius);
    const Vector3f lp = (p - origin) / r;
    return fractal_power_distance_estimator(lp, Vector3f(0.0f, 0.0f, 0.0f), iterations, power, bailout, false) * r;
}

void Mandelbulb::calc_aabb()
{
    const scalar_t r = std::max((scalar_t)EPSILON, radius);
    aabb.min = origin - Vector3f(r, r, r);
    aabb.max = origin + Vector3f(r, r, r);
}

Vector3f Mandelbulb::point_sample() const
{
    return origin;
}

Ray Mandelbulb::ray_sample() const
{
    Ray ray;
    ray.origin = origin;
    ray.direction = Vector3f(0.0f, 1.0f, 0.0f);
    return ray;
}

Vector3f Mandelbulb::emitter_position() const
{
    return origin;
}

JuliaFractal::JuliaFractal()
    : origin(0.0f, 0.0f, 0.0f)
    , julia_c(-0.24f, 0.74f, 0.12f)
    , radius(1.0f)
    , iterations(18)
    , power(8.0f)
    , bailout(4.0f)
{}

bool JuliaFractal::intersection(const Ray &ray, hit_record_t *i_hit_record) const
{
    return intersect_raymarch(*this, ray, std::max((scalar_t)EPSILON, radius), (scalar_t)0.0010, 240, i_hit_record);
}

nmath::scalar_t JuliaFractal::distance(nmath::Vector3f p) const
{
    const scalar_t r = std::max((scalar_t)EPSILON, radius);
    const Vector3f lp = (p - origin) / r;
    return fractal_power_distance_estimator(lp, julia_c, iterations, power, bailout, true) * r;
}

void JuliaFractal::calc_aabb()
{
    const scalar_t r = std::max((scalar_t)EPSILON, radius);
    aabb.min = origin - Vector3f(r, r, r);
    aabb.max = origin + Vector3f(r, r, r);
}

Vector3f JuliaFractal::point_sample() const
{
    return origin;
}

Ray JuliaFractal::ray_sample() const
{
    Ray ray;
    ray.origin = origin;
    ray.direction = Vector3f(0.0f, 1.0f, 0.0f);
    return ray;
}

Vector3f JuliaFractal::emitter_position() const
{
    return origin;
}

} /* namespace surface */
} /* namespace xtcore */
