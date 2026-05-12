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

// Orbit data for power-law fractals (Mandelbulb, Julia).
// Returns vec3(smooth_iter, sphere_trap, plane_trap_y), each in [0,1].
// smooth_iter: continuous escape speed — eliminates iteration-band banding.
// sphere_trap: minimum |z| across all orbit steps, normalized by bailout.
// plane_trap_y: minimum |z.y| across all orbit steps, normalized by bailout.
static Vector3f orbit_power_fractal(const Vector3f &p,
                                    const Vector3f &c,
                                    size_t iterations,
                                    scalar_t power,
                                    scalar_t bailout,
                                    bool use_seed_c)
{
    Vector3f z = p;
    scalar_t r = 0;
    scalar_t min_r  = INFINITY;
    scalar_t min_py = INFINITY;
    size_t   i      = 0;

    const size_t iters = std::max((size_t)1, iterations);
    const scalar_t pw  = std::max((scalar_t)2.0, power);
    const scalar_t bo  = std::max((scalar_t)2.0, bailout);

    for (i = 0; i < iters; ++i) {
        r = z.length();
        if (r > bo) break;
        if (r < (scalar_t)1e-9) { z = use_seed_c ? c : p; continue; }

        min_r  = std::min(min_r,  r);
        min_py = std::min(min_py, nmath_abs(z.y));

        const scalar_t theta = (scalar_t)std::acos((double)clamp_signed_unit(z.z / r));
        const scalar_t phi   = (scalar_t)std::atan2((double)z.y, (double)z.x);
        const scalar_t zr    = (scalar_t)std::pow((double)r, (double)pw);
        const scalar_t t     = theta * pw;
        const scalar_t ph    = phi   * pw;
        const scalar_t st    = (scalar_t)std::sin((double)t);
        z = Vector3f(
            zr * st * (scalar_t)std::cos((double)ph),
            zr * st * (scalar_t)std::sin((double)ph),
            zr *      (scalar_t)std::cos((double)t)
        ) + (use_seed_c ? c : p);
    }

    // Smooth/continuous escape count: removes hard banding between integer iterations.
    scalar_t smooth = (scalar_t)i;
    if (i < iters && std::isfinite((double)r) && r > (scalar_t)1.0) {
        const double log_r  = std::log((double)r);
        const double log_bo = std::log((double)bo);
        if (log_r > 1e-10 && log_bo > 1e-10)
            smooth = (scalar_t)i + 1.0f - (scalar_t)(std::log2(log_r / log_bo));
    }

    if (!std::isfinite((double)min_r))  min_r  = bo;
    if (!std::isfinite((double)min_py)) min_py = bo;

    return Vector3f(
        clamp_unit(smooth / (scalar_t)iters),
        clamp_unit(min_r  / bo),
        clamp_unit(min_py / bo)
    );
}

// ---------- MandelBox DE ----------
static scalar_t mandelbox_de(const Vector3f &p,
                              scalar_t fold,
                              scalar_t min_r2,
                              scalar_t fixed_r2,
                              scalar_t scale,
                              size_t   iters,
                              scalar_t bailout)
{
    const scalar_t bo2 = bailout * bailout;
    Vector3f z = p;
    scalar_t dr = 1.0f;

    for (size_t i = 0; i < iters; ++i) {
        // Box fold
        if      (z.x >  fold) z.x =  2.0f * fold - z.x;
        else if (z.x < -fold) z.x = -2.0f * fold - z.x;
        if      (z.y >  fold) z.y =  2.0f * fold - z.y;
        else if (z.y < -fold) z.y = -2.0f * fold - z.y;
        if      (z.z >  fold) z.z =  2.0f * fold - z.z;
        else if (z.z < -fold) z.z = -2.0f * fold - z.z;

        // Ball fold
        const scalar_t r2 = z.x*z.x + z.y*z.y + z.z*z.z;
        if (r2 < min_r2) {
            const scalar_t t = fixed_r2 / min_r2;
            z = z * t;
            dr *= t;
        } else if (r2 < fixed_r2) {
            const scalar_t t = fixed_r2 / r2;
            z = z * t;
            dr *= t;
        }

        z = z * scale + p;
        dr = dr * nmath_abs(scale) + 1.0f;

        if (z.x*z.x + z.y*z.y + z.z*z.z > bo2) break;
    }

    const scalar_t sz = z.length();
    const scalar_t adr = nmath_abs(dr);
    if (!std::isfinite((double)sz) || adr <= EPSILON) return 1e6f;
    const scalar_t border = std::max((scalar_t)0.0, nmath_abs(scale) - 1.0f);
    return (sz - border) / adr;
}

// Orbit trap for MandelBox: box-trap + sphere-trap + min-r trap.
static Vector3f orbit_mandelbox(const Vector3f &p,
                                 scalar_t fold,
                                 scalar_t min_r2,
                                 scalar_t fixed_r2,
                                 scalar_t scale,
                                 size_t   iters,
                                 scalar_t bailout)
{
    const scalar_t bo2 = bailout * bailout;
    Vector3f z = p;
    scalar_t min_r = INFINITY;
    scalar_t min_box = INFINITY;
    size_t escape_i = iters;

    for (size_t i = 0; i < iters; ++i) {
        if (z.x >  fold) z.x =  2.0f * fold - z.x;
        else if (z.x < -fold) z.x = -2.0f * fold - z.x;
        if (z.y >  fold) z.y =  2.0f * fold - z.y;
        else if (z.y < -fold) z.y = -2.0f * fold - z.y;
        if (z.z >  fold) z.z =  2.0f * fold - z.z;
        else if (z.z < -fold) z.z = -2.0f * fold - z.z;

        const scalar_t r2 = z.x*z.x + z.y*z.y + z.z*z.z;
        min_r = std::min(min_r, nmath_sqrt(r2));
        const scalar_t bx = std::max({nmath_abs(z.x), nmath_abs(z.y), nmath_abs(z.z)});
        min_box = std::min(min_box, bx);

        if (r2 < min_r2) {
            z = z * (fixed_r2 / min_r2);
        } else if (r2 < fixed_r2) {
            z = z * (fixed_r2 / r2);
        }

        z = z * scale + p;

        if (z.x*z.x + z.y*z.y + z.z*z.z > bo2) {
            escape_i = i;
            break;
        }
    }

    if (!std::isfinite((double)min_r))  min_r  = nmath_sqrt(bo2);
    if (!std::isfinite((double)min_box)) min_box = fold;

    return Vector3f(
        clamp_unit((scalar_t)escape_i / (scalar_t)iters),
        clamp_unit(min_r  / (nmath_sqrt(bo2))),
        clamp_unit(min_box / fold)
    );
}

// ---------- Quaternion Julia DE ----------
struct Quat { scalar_t w, x, y, z; };
static Quat quat_sq_add(Quat q, Quat c)
{
    return Quat{
        q.w*q.w - q.x*q.x - q.y*q.y - q.z*q.z + c.w,
        2.0f*q.w*q.x + c.x,
        2.0f*q.w*q.y + c.y,
        2.0f*q.w*q.z + c.z
    };
}

static scalar_t quat_julia_de(const Vector3f &p,
                               const Vector3f &c_xyz,
                               scalar_t c_w,
                               scalar_t w0,
                               size_t iters,
                               scalar_t bailout)
{
    Quat q = {p.x, p.y, p.z, w0};
    const Quat c = {c_w, c_xyz.x, c_xyz.y, c_xyz.z};
    scalar_t dr = 1.0f;
    const scalar_t bo2 = bailout * bailout;

    for (size_t i = 0; i < iters; ++i) {
        const scalar_t r2 = q.w*q.w + q.x*q.x + q.y*q.y + q.z*q.z;
        if (r2 > bo2) {
            const scalar_t r = nmath_sqrt(r2);
            if (dr <= EPSILON || r <= 1.0f) return 0.0f;
            return 0.5f * r * (scalar_t)std::log((double)r) / dr;
        }
        dr = 2.0f * nmath_sqrt(r2) * dr;
        q = quat_sq_add(q, c);
    }
    return 0.0f;
}

static Vector3f orbit_quat_julia(const Vector3f &p,
                                  const Vector3f &c_xyz,
                                  scalar_t c_w,
                                  scalar_t w0,
                                  size_t iters,
                                  scalar_t bailout)
{
    Quat q = {p.x, p.y, p.z, w0};
    const Quat c = {c_w, c_xyz.x, c_xyz.y, c_xyz.z};
    const scalar_t bo2 = bailout * bailout;
    scalar_t min_r = INFINITY;
    scalar_t min_py = INFINITY;
    size_t escape_i = iters;

    for (size_t i = 0; i < iters; ++i) {
        const scalar_t r2 = q.w*q.w + q.x*q.x + q.y*q.y + q.z*q.z;
        min_r  = std::min(min_r,  nmath_sqrt(r2));
        min_py = std::min(min_py, nmath_abs(q.y));
        if (r2 > bo2) { escape_i = i; break; }
        q = quat_sq_add(q, c);
    }

    if (!std::isfinite((double)min_r))  min_r  = bailout;
    if (!std::isfinite((double)min_py)) min_py = bailout;

    return Vector3f(
        clamp_unit((scalar_t)escape_i / (scalar_t)iters),
        clamp_unit(min_r  / bailout),
        clamp_unit(min_py / bailout)
    );
}

// ---------- Burning Ship 3D DE ----------
static scalar_t burning_ship_3d_de(const Vector3f &p,
                                    size_t iters,
                                    scalar_t power,
                                    scalar_t bailout)
{
    Vector3f z = p;
    scalar_t dr = 1.0f;
    scalar_t r  = 0.0f;
    const scalar_t pw = std::max((scalar_t)2.0, power);
    const scalar_t bo = std::max((scalar_t)2.0, bailout);

    for (size_t i = 0; i < iters; ++i) {
        r = z.length();
        if (r > bo) break;
        if (r < (scalar_t)1e-9) { z = p; continue; }

        // Burning Ship: abs on x and y before computing angle
        const scalar_t ax = nmath_abs(z.x);
        const scalar_t ay = nmath_abs(z.y);
        const scalar_t az = z.z;

        const scalar_t theta = (scalar_t)std::acos((double)clamp_signed_unit(az / r));
        const scalar_t phi   = (scalar_t)std::atan2((double)ay, (double)ax);
        const scalar_t zr_pw_1 = (scalar_t)std::pow((double)r, (double)(pw - 1.0f));
        dr = zr_pw_1 * pw * dr + 1.0f;

        const scalar_t zr = zr_pw_1 * r;
        const scalar_t t  = theta * pw;
        const scalar_t ph = phi   * pw;
        const scalar_t st = (scalar_t)std::sin((double)t);
        z = Vector3f(
            zr * st * (scalar_t)std::cos((double)ph),
            zr * st * (scalar_t)std::sin((double)ph),
            zr *      (scalar_t)std::cos((double)t)
        ) + p;
    }

    r = z.length();
    if (!std::isfinite((double)r) || !std::isfinite((double)dr) || dr <= EPSILON) return 1e6f;
    if (r <= EPSILON) return 0.0f;
    return (scalar_t)(0.5 * std::log((double)r) * (double)r / (double)dr);
}

// ---------- Cantor Dust 3D recursive ----------
static void intersect_cantor_recursive(const Ray &ray,
                                        const Vector3f &center,
                                        scalar_t half,
                                        size_t depth,
                                        fractal_hit_t &best)
{
    const Vector3f bmin = center - Vector3f(half, half, half);
    const Vector3f bmax = center + Vector3f(half, half, half);
    scalar_t t_near = 0.0f, t_far = 0.0f;
    if (!ray_aabb_hit(ray, bmin, bmax, &t_near, &t_far)) return;
    if (t_near > best.t) return;

    if (depth == 0) {
        scalar_t t = 0.0f;
        Vector3f n;
        if (ray_aabb_hit_with_normal(ray, bmin, bmax, t, n) && t < best.t) {
            best.hit = true;
            best.t   = t;
            best.normal = n;
        }
        return;
    }

    // Keep only the 8 corners: skip the middle third on every axis
    const scalar_t child_half = half / 3.0f;
    for (int ix = -1; ix <= 1; ix += 2) {
        for (int iy = -1; iy <= 1; iy += 2) {
            for (int iz = -1; iz <= 1; iz += 2) {
                const Vector3f child_center = center + Vector3f(
                    (scalar_t)ix, (scalar_t)iy, (scalar_t)iz
                ) * (2.0f * child_half);
                intersect_cantor_recursive(ray, child_center, child_half, depth - 1, best);
            }
        }
    }
}

// ---------- Icosahedral IFS DE ----------
static const scalar_t ICO_PHI = 1.6180339887f;

// Reflect z across each of the 6 icosahedral symmetry planes once.
// The fold planes are defined by the 6 dodecahedron face normals:
//   ±(1, phi, 0),  ±(0, 1, phi),  ±(phi, 0, 1)
// Any point gets pushed toward the positive fundamental domain.
static void icosa_fold_pass(Vector3f &z)
{
    const scalar_t phi = ICO_PHI;
    // Unnormalized normals — reflect p if dot(p,n) < 0
    static const Vector3f folds[6] = {
        Vector3f( 1.0f,  phi,  0.0f),
        Vector3f(-1.0f,  phi,  0.0f),
        Vector3f( 0.0f,  1.0f,  phi),
        Vector3f( 0.0f, -1.0f,  phi),
        Vector3f( phi,  0.0f,  1.0f),
        Vector3f(-phi,  0.0f,  1.0f)
    };
    static const scalar_t len2[6] = {
        1.0f + phi*phi, 1.0f + phi*phi,
        1.0f + phi*phi, 1.0f + phi*phi,
        phi*phi + 1.0f, phi*phi + 1.0f
    };
    for (int i = 0; i < 6; ++i) {
        const scalar_t t = nmath::dot(z, folds[i]);
        if (t < 0.0f) z -= folds[i] * (2.0f * t / len2[i]);
    }
}

// MandelBox-style escape DE using icosahedral fold planes in place of the box fold.
// Icosahedral fold (reflection) replaces box-fold clamping; ball fold and +c term are
// identical to MandelBox so the same escape structure and DE formula apply.
static scalar_t icosahedral_ifs_de(const Vector3f &pos,
                                    size_t iters,
                                    scalar_t scale_param,
                                    scalar_t radius)
{
    Vector3f z = pos / radius;
    const Vector3f c = z;           // original point added back each step
    scalar_t dr = 1.0f;
    const scalar_t s    = scale_param;
    const scalar_t abs_s = nmath_abs(s);

    for (size_t i = 0; i < iters; ++i) {
        // Icosahedral fold (4 passes per iteration for symmetry convergence)
        icosa_fold_pass(z);
        icosa_fold_pass(z);
        icosa_fold_pass(z);
        icosa_fold_pass(z);

        // Ball fold: fixed_r=1, min_r=0.5 (same as default MandelBox)
        const scalar_t r2 = z.x*z.x + z.y*z.y + z.z*z.z;
        if (r2 < 0.25f) {           // inner sphere: scale up by fixed_r^2/min_r^2 = 4
            z  = z * 4.0f;
            dr *= 4.0f;
        } else if (r2 < 1.0f) {     // annular region: invert magnitude
            z  = z / r2;
            dr /= r2;
        }

        // Scale and add original point (MandelBox escape formula)
        z  = z * s + c;
        dr = dr * abs_s + 1.0f;

        if (z.x*z.x + z.y*z.y + z.z*z.z > 1024.0f) break;
    }

    const scalar_t sz  = z.length();
    const scalar_t adr = nmath_abs(dr);
    if (!std::isfinite((double)sz) || adr <= EPSILON) return 1e6f;
    const scalar_t border = std::max((scalar_t)0.0, abs_s - 1.0f);
    return (sz - border) / adr * radius;
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
    , orbit_trap_channel(1)
{}

bool Mandelbulb::intersection(const Ray &ray, hit_record_t *i_hit_record) const
{
    const bool hit = intersect_raymarch(*this, ray, std::max((scalar_t)EPSILON, radius), (scalar_t)0.0012, 220, i_hit_record);
    if (hit && i_hit_record) {
        const scalar_t r = std::max((scalar_t)EPSILON, radius);
        const Vector3f lp = (i_hit_record->point - origin) / r;
        const Vector3f trap = orbit_power_fractal(lp, Vector3f(0,0,0), iterations, power, bailout, false);
        i_hit_record->texcoord = trap;
        i_hit_record->material_selector = (&trap.x)[orbit_trap_channel];
    }
    return hit;
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
    , orbit_trap_channel(1)
{}

bool JuliaFractal::intersection(const Ray &ray, hit_record_t *i_hit_record) const
{
    const bool hit = intersect_raymarch(*this, ray, std::max((scalar_t)EPSILON, radius), (scalar_t)0.0010, 240, i_hit_record);
    if (hit && i_hit_record) {
        const scalar_t r = std::max((scalar_t)EPSILON, radius);
        const Vector3f lp = (i_hit_record->point - origin) / r;
        const Vector3f trap = orbit_power_fractal(lp, julia_c, iterations, power, bailout, true);
        i_hit_record->texcoord = trap;
        i_hit_record->material_selector = (&trap.x)[orbit_trap_channel];
    }
    return hit;
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

// ============================================================
// MandelBox
// ============================================================
MandelBox::MandelBox()
    : origin(0.0f, 0.0f, 0.0f)
    , radius(1.0f)
    , iterations(16)
    , fold_size(1.0f)
    , min_r(0.5f)
    , scale(-2.5f)
    , bailout(100.0f)
    , orbit_trap_channel(1)
{}

bool MandelBox::intersection(const Ray &ray, hit_record_t *i_hit_record) const
{
    const bool hit = intersect_raymarch(*this, ray, std::max((scalar_t)EPSILON, radius), (scalar_t)0.0015, 180, i_hit_record);
    if (hit && i_hit_record) {
        const scalar_t r  = std::max((scalar_t)EPSILON, radius);
        const Vector3f lp = (i_hit_record->point - origin) / r;
        const scalar_t fixed_r2 = 1.0f;
        const scalar_t min_r2   = min_r * min_r;
        const Vector3f trap = orbit_mandelbox(lp, fold_size, min_r2, fixed_r2, scale, iterations, bailout);
        i_hit_record->texcoord = trap;
        i_hit_record->material_selector = (&trap.x)[orbit_trap_channel];
    }
    return hit;
}

nmath::scalar_t MandelBox::distance(nmath::Vector3f p) const
{
    const scalar_t r      = std::max((scalar_t)EPSILON, radius);
    const Vector3f lp     = (p - origin) / r;
    const scalar_t fixed_r2 = 1.0f;
    const scalar_t min_r2   = min_r * min_r;
    return mandelbox_de(lp, fold_size, min_r2, fixed_r2, scale, iterations, bailout) * r;
}

void MandelBox::calc_aabb()
{
    const scalar_t r = std::max((scalar_t)EPSILON, radius);
    // MandelBox attractor fits within roughly 3 world-units at scale -2.5
    const scalar_t pad = r * 3.0f;
    aabb.min = origin - Vector3f(pad, pad, pad);
    aabb.max = origin + Vector3f(pad, pad, pad);
}

Vector3f MandelBox::point_sample() const { return origin; }

Ray MandelBox::ray_sample() const
{
    Ray ray;
    ray.origin    = origin;
    ray.direction = Vector3f(0.0f, 1.0f, 0.0f);
    return ray;
}

Vector3f MandelBox::emitter_position() const { return origin; }

// ============================================================
// QuaternionJulia
// ============================================================
QuaternionJulia::QuaternionJulia()
    : origin(0.0f, 0.0f, 0.0f)
    , quat_c(-0.2f, 0.6f, 0.2f)
    , quat_cw(-0.1f)
    , quat_w0(0.0f)
    , radius(1.0f)
    , iterations(12)
    , bailout(4.0f)
    , orbit_trap_channel(1)
{}

bool QuaternionJulia::intersection(const Ray &ray, hit_record_t *i_hit_record) const
{
    const bool hit = intersect_raymarch(*this, ray, std::max((scalar_t)EPSILON, radius), (scalar_t)0.0010, 240, i_hit_record);
    if (hit && i_hit_record) {
        const scalar_t r  = std::max((scalar_t)EPSILON, radius);
        const Vector3f lp = (i_hit_record->point - origin) / r;
        const Vector3f trap = orbit_quat_julia(lp, quat_c, quat_cw, quat_w0, iterations, bailout);
        i_hit_record->texcoord = trap;
        i_hit_record->material_selector = (&trap.x)[orbit_trap_channel];
    }
    return hit;
}

nmath::scalar_t QuaternionJulia::distance(nmath::Vector3f p) const
{
    const scalar_t r  = std::max((scalar_t)EPSILON, radius);
    const Vector3f lp = (p - origin) / r;
    return quat_julia_de(lp, quat_c, quat_cw, quat_w0, iterations, bailout) * r;
}

void QuaternionJulia::calc_aabb()
{
    const scalar_t r = std::max((scalar_t)EPSILON, radius);
    aabb.min = origin - Vector3f(r, r, r);
    aabb.max = origin + Vector3f(r, r, r);
}

Vector3f QuaternionJulia::point_sample() const { return origin; }

Ray QuaternionJulia::ray_sample() const
{
    Ray ray;
    ray.origin    = origin;
    ray.direction = Vector3f(0.0f, 1.0f, 0.0f);
    return ray;
}

Vector3f QuaternionJulia::emitter_position() const { return origin; }

// ============================================================
// BurningShip3D
// ============================================================
BurningShip3D::BurningShip3D()
    : origin(0.0f, 0.0f, 0.0f)
    , radius(1.0f)
    , iterations(18)
    , power(2.0f)
    , bailout(4.0f)
    , orbit_trap_channel(1)
{}

bool BurningShip3D::intersection(const Ray &ray, hit_record_t *i_hit_record) const
{
    const bool hit = intersect_raymarch(*this, ray, std::max((scalar_t)EPSILON, radius), (scalar_t)0.0012, 220, i_hit_record);
    if (hit && i_hit_record) {
        const scalar_t r  = std::max((scalar_t)EPSILON, radius);
        const Vector3f lp = (i_hit_record->point - origin) / r;
        // Reuse power-fractal orbit: abs folds make the orbit statistics similar
        const Vector3f trap = orbit_power_fractal(lp, Vector3f(0,0,0), iterations, power, bailout, false);
        i_hit_record->texcoord = trap;
        i_hit_record->material_selector = (&trap.x)[orbit_trap_channel];
    }
    return hit;
}

nmath::scalar_t BurningShip3D::distance(nmath::Vector3f p) const
{
    const scalar_t r  = std::max((scalar_t)EPSILON, radius);
    const Vector3f lp = (p - origin) / r;
    return burning_ship_3d_de(lp, iterations, power, bailout) * r;
}

void BurningShip3D::calc_aabb()
{
    const scalar_t r = std::max((scalar_t)EPSILON, radius);
    aabb.min = origin - Vector3f(r, r, r);
    aabb.max = origin + Vector3f(r, r, r);
}

Vector3f BurningShip3D::point_sample() const { return origin; }

Ray BurningShip3D::ray_sample() const
{
    Ray ray;
    ray.origin    = origin;
    ray.direction = Vector3f(0.0f, 1.0f, 0.0f);
    return ray;
}

Vector3f BurningShip3D::emitter_position() const { return origin; }

// ============================================================
// CantorDust3D
// ============================================================
CantorDust3D::CantorDust3D()
    : origin(0.0f, 0.0f, 0.0f)
    , orientation(0.0f, 0.0f, 0.0f)
    , radius(1.0f)
    , iterations(3)
{}

bool CantorDust3D::intersection(const Ray &ray, hit_record_t *i_hit_record) const
{
    Ray local_ray;
    local_ray.origin    = inverse_rotate_euler_xyz(ray.origin    - origin, orientation);
    local_ray.direction = inverse_rotate_euler_xyz(ray.direction,          orientation).normalized();

    fractal_hit_t hit;
    const size_t depth = std::max((size_t)1, std::min((size_t)6, iterations));
    intersect_cantor_recursive(local_ray, Vector3f(0.0f, 0.0f, 0.0f),
                               std::max((scalar_t)EPSILON, radius), depth, hit);
    if (!hit.hit || hit.t <= EPSILON) return false;

    if (i_hit_record) {
        i_hit_record->t = hit.t;
        const Vector3f local_point  = local_ray.origin + local_ray.direction * hit.t;
        const Vector3f world_point  = rotate_euler_xyz(local_point, orientation) + origin;
        Vector3f world_normal = rotate_euler_xyz(hit.normal, orientation).normalized();
        if (nmath::dot(world_normal, ray.direction) > 0.0f) world_normal = -world_normal;
        i_hit_record->point   = world_point;
        i_hit_record->normal  = world_normal;
        const Vector3f lp = local_point / std::max((scalar_t)EPSILON, radius);
        i_hit_record->texcoord = Vector3f(lp.x * 0.5f + 0.5f, lp.y * 0.5f + 0.5f, 0.0f);
        i_hit_record->incident_direction = ray.direction;
    }
    return true;
}

nmath::scalar_t CantorDust3D::distance(nmath::Vector3f p) const
{
    const scalar_t r  = std::max((scalar_t)EPSILON, radius);
    const Vector3f lp = inverse_rotate_euler_xyz(p - origin, orientation) / r;
    return sd_box_unit(lp) * r;
}

void CantorDust3D::calc_aabb()
{
    const scalar_t r = std::max((scalar_t)EPSILON, radius);
    const Vector3f corners[8] = {
        Vector3f(-r,-r,-r), Vector3f(-r,-r, r),
        Vector3f(-r, r,-r), Vector3f(-r, r, r),
        Vector3f( r,-r,-r), Vector3f( r,-r, r),
        Vector3f( r, r,-r), Vector3f( r, r, r)
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

Vector3f CantorDust3D::point_sample() const { return origin; }

Ray CantorDust3D::ray_sample() const
{
    Ray ray;
    ray.origin    = origin;
    ray.direction = Vector3f(0.0f, 1.0f, 0.0f);
    return ray;
}

Vector3f CantorDust3D::emitter_position() const { return origin; }

// ============================================================
// IcosahedralIFS
// ============================================================
IcosahedralIFS::IcosahedralIFS()
    : origin(0.0f, 0.0f, 0.0f)
    , radius(1.0f)
    , iterations(10)
    , scale(-2.5f)
{}

bool IcosahedralIFS::intersection(const Ray &ray, hit_record_t *i_hit_record) const
{
    return intersect_raymarch(*this, ray, std::max((scalar_t)EPSILON, radius) * 3.0f,
                              (scalar_t)0.0015, 200, i_hit_record);
}

nmath::scalar_t IcosahedralIFS::distance(nmath::Vector3f p) const
{
    const scalar_t r = std::max((scalar_t)EPSILON, radius);
    return icosahedral_ifs_de(p - origin, iterations, scale, r);
}

void IcosahedralIFS::calc_aabb()
{
    const scalar_t r   = std::max((scalar_t)EPSILON, radius);
    const scalar_t pad = r * 3.0f;
    aabb.min = origin - Vector3f(pad, pad, pad);
    aabb.max = origin + Vector3f(pad, pad, pad);
}

Vector3f IcosahedralIFS::point_sample() const { return origin; }

Ray IcosahedralIFS::ray_sample() const
{
    Ray ray;
    ray.origin    = origin;
    ray.direction = Vector3f(0.0f, 1.0f, 0.0f);
    return ray;
}

Vector3f IcosahedralIFS::emitter_position() const { return origin; }

} /* namespace surface */
} /* namespace xtcore */
