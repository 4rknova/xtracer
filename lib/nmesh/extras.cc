#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <nmath/precision.h>
#include <nmath/vector.h>

#include "extras.h"

namespace nmesh {
    namespace generator {

namespace {

typedef nmath::Vector3f Vec3;

typedef struct {
    int a;
    int b;
    int c;
} tri_t;

typedef struct {
    float u;
    float v;
} uv_t;

typedef struct {
    int x;
    int y;
    int z;
} ivec3_t;

static int clampi(int v, int lo, int hi)
{
    return std::max(lo, std::min(v, hi));
}

static int ipowi(int base, int exp)
{
    int out = 1;
    for (int i = 0; i < exp; ++i) out *= base;
    return out;
}

static tri_t make_tri(int a, int b, int c)
{
    tri_t t;
    t.a = a;
    t.b = b;
    t.c = c;
    return t;
}

static int append_vertex(object_t *obj, const Vec3 &p, const Vec3 &n, const uv_t *uv = 0)
{
    const int idx = (int)(obj->attributes.v.size() / 3);
    obj->attributes.v.push_back(p.x);
    obj->attributes.v.push_back(p.y);
    obj->attributes.v.push_back(p.z);

    obj->attributes.n.push_back(n.x);
    obj->attributes.n.push_back(n.y);
    obj->attributes.n.push_back(n.z);

    if (uv) {
        obj->attributes.uv.push_back(uv->u);
        obj->attributes.uv.push_back(uv->v);
    }
    return idx;
}

static void append_triangle(shape_t &shape, int a, int b, int c, bool has_uv = false)
{
    index_t ia;
    ia.v = a; ia.n = a; ia.uv = has_uv ? a : -1;
    index_t ib;
    ib.v = b; ib.n = b; ib.uv = has_uv ? b : -1;
    index_t ic;
    ic.v = c; ic.n = c; ic.uv = has_uv ? c : -1;

    shape.mesh.indices.push_back(ia);
    shape.mesh.indices.push_back(ib);
    shape.mesh.indices.push_back(ic);
}

static void append_quad(shape_t &shape, int a, int b, int c, int d, bool has_uv = false)
{
    append_triangle(shape, a, b, c, has_uv);
    append_triangle(shape, a, c, d, has_uv);
}

static void build_icosphere_data(std::vector<Vec3> &verts, std::vector<tri_t> &faces, int iterations)
{
    const float k = (1.f + nmath_sqrt(5.f)) * 0.5f;

    verts.clear();
    verts.push_back(Vec3(-1, k, 0).normalized());
    verts.push_back(Vec3(1, k, 0).normalized());
    verts.push_back(Vec3(-1, -k, 0).normalized());
    verts.push_back(Vec3(1, -k, 0).normalized());
    verts.push_back(Vec3(0, -1, k).normalized());
    verts.push_back(Vec3(0, 1, k).normalized());
    verts.push_back(Vec3(0, -1, -k).normalized());
    verts.push_back(Vec3(0, 1, -k).normalized());
    verts.push_back(Vec3(k, 0, -1).normalized());
    verts.push_back(Vec3(k, 0, 1).normalized());
    verts.push_back(Vec3(-k, 0, -1).normalized());
    verts.push_back(Vec3(-k, 0, 1).normalized());

    const int f[] = {
        0,11,5, 0,5,1, 0,1,7, 0,7,10, 0,10,11,
        1,5,9, 5,11,4, 11,10,2, 10,7,6, 7,1,8,
        3,9,4, 3,4,2, 3,2,6, 3,6,8, 3,8,9,
        4,9,5, 2,4,11, 6,2,10, 8,6,7, 9,8,1
    };

    faces.clear();
    for (size_t i = 0; i < sizeof(f) / sizeof(f[0]); i += 3) {
        tri_t t;
        t.a = f[i];
        t.b = f[i + 1];
        t.c = f[i + 2];
        faces.push_back(t);
    }

    iterations = clampi(iterations, 0, 7);
    for (int it = 0; it < iterations; ++it) {
        std::map<std::pair<int, int>, int> edge_mid;
        std::vector<tri_t> next_faces;
        next_faces.reserve(faces.size() * 4);

        auto midpoint = [&](int a, int b) -> int {
            const int lo = std::min(a, b);
            const int hi = std::max(a, b);
            const std::pair<int, int> key(lo, hi);
            std::map<std::pair<int, int>, int>::iterator found = edge_mid.find(key);
            if (found != edge_mid.end()) return found->second;

            const Vec3 m = (verts[a] + verts[b]).normalized();
            const int idx = (int)verts.size();
            verts.push_back(m);
            edge_mid[key] = idx;
            return idx;
        };

        for (size_t i = 0; i < faces.size(); ++i) {
            const tri_t t = faces[i];
            const int ab = midpoint(t.a, t.b);
            const int bc = midpoint(t.b, t.c);
            const int ca = midpoint(t.c, t.a);

            next_faces.push_back(make_tri(t.a, ab, ca));
            next_faces.push_back(make_tri(t.b, bc, ab));
            next_faces.push_back(make_tri(t.c, ca, bc));
            next_faces.push_back(make_tri(ab, bc, ca));
        }

        faces.swap(next_faces);
    }
}

static void add_triangle_flat(object_t *obj, shape_t &shape, const Vec3 &a, const Vec3 &b, const Vec3 &c)
{
    Vec3 n = nmath::cross(b - a, c - a);
    if (n.length() <= 1e-8f) return;
    n.normalize();

    const int ia = append_vertex(obj, a, n);
    const int ib = append_vertex(obj, b, n);
    const int ic = append_vertex(obj, c, n);
    append_triangle(shape, ia, ib, ic, false);
}

static void add_cube_faces(object_t *obj, shape_t &shape, const Vec3 &center, float size,
                           const std::set<std::string> &occupied, int gx, int gy, int gz)
{
    const float h = size * 0.5f;

    auto key = [](int x, int y, int z) -> std::string {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%d,%d,%d", x, y, z);
        return std::string(buf);
    };

    const Vec3 p000(center.x - h, center.y - h, center.z - h);
    const Vec3 p100(center.x + h, center.y - h, center.z - h);
    const Vec3 p110(center.x + h, center.y + h, center.z - h);
    const Vec3 p010(center.x - h, center.y + h, center.z - h);
    const Vec3 p001(center.x - h, center.y - h, center.z + h);
    const Vec3 p101(center.x + h, center.y - h, center.z + h);
    const Vec3 p111(center.x + h, center.y + h, center.z + h);
    const Vec3 p011(center.x - h, center.y + h, center.z + h);

    if (occupied.find(key(gx + 1, gy, gz)) == occupied.end()) {
        // +X
        add_triangle_flat(obj, shape, p100, p110, p111);
        add_triangle_flat(obj, shape, p100, p111, p101);
    }
    if (occupied.find(key(gx - 1, gy, gz)) == occupied.end()) {
        // -X
        add_triangle_flat(obj, shape, p001, p011, p010);
        add_triangle_flat(obj, shape, p001, p010, p000);
    }
    if (occupied.find(key(gx, gy + 1, gz)) == occupied.end()) {
        // +Y
        add_triangle_flat(obj, shape, p010, p011, p111);
        add_triangle_flat(obj, shape, p010, p111, p110);
    }
    if (occupied.find(key(gx, gy - 1, gz)) == occupied.end()) {
        // -Y
        add_triangle_flat(obj, shape, p000, p100, p101);
        add_triangle_flat(obj, shape, p000, p101, p001);
    }
    if (occupied.find(key(gx, gy, gz + 1)) == occupied.end()) {
        // +Z
        add_triangle_flat(obj, shape, p101, p111, p011);
        add_triangle_flat(obj, shape, p101, p011, p001);
    }
    if (occupied.find(key(gx, gy, gz - 1)) == occupied.end()) {
        // -Z
        add_triangle_flat(obj, shape, p000, p010, p110);
        add_triangle_flat(obj, shape, p000, p110, p100);
    }
}

static std::string voxel_key(int x, int y, int z)
{
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%d,%d,%d", x, y, z);
    return std::string(buf);
}

static float wrap_unit(float v)
{
    while (v > 1.0f) v -= 2.0f;
    while (v < -1.0f) v += 2.0f;
    return v;
}

static bool inside_menger_local(Vec3 p, int depth)
{
    if (std::fabs(p.x) > 1.0f || std::fabs(p.y) > 1.0f || std::fabs(p.z) > 1.0f) return false;
    depth = std::max(1, depth);

    for (int i = 0; i < depth; ++i) {
        const Vec3 a(std::fabs(p.x), std::fabs(p.y), std::fabs(p.z));
        const bool cut_xy = (a.x < (1.0f / 3.0f) && a.y < (1.0f / 3.0f));
        const bool cut_xz = (a.x < (1.0f / 3.0f) && a.z < (1.0f / 3.0f));
        const bool cut_yz = (a.y < (1.0f / 3.0f) && a.z < (1.0f / 3.0f));
        if (cut_xy || cut_xz || cut_yz) return false;

        p.x = wrap_unit(p.x * 3.0f);
        p.y = wrap_unit(p.y * 3.0f);
        p.z = wrap_unit(p.z * 3.0f);
    }

    return true;
}

static bool tetra_barycentric(const Vec3 &p, const Vec3 (&v)[4], float (&bary)[4])
{
    const Vec3 e1 = v[1] - v[0];
    const Vec3 e2 = v[2] - v[0];
    const Vec3 e3 = v[3] - v[0];
    const Vec3 r = p - v[0];

    const Vec3 c23 = nmath::cross(e2, e3);
    const Vec3 cr3 = nmath::cross(r, e3);
    const Vec3 c2r = nmath::cross(e2, r);
    const float det = nmath::dot(e1, c23);
    if (std::fabs(det) <= 1e-8f) return false;

    const float inv_det = 1.0f / det;
    const float b1 = nmath::dot(r, c23) * inv_det;
    const float b2 = nmath::dot(e1, cr3) * inv_det;
    const float b3 = nmath::dot(e1, c2r) * inv_det;

    bary[0] = 1.0f - b1 - b2 - b3;
    bary[1] = b1;
    bary[2] = b2;
    bary[3] = b3;
    return true;
}

static bool inside_sierpinski_tetra_local(const Vec3 &p, int depth)
{
    const float s = 1.0f / nmath_sqrt(3.0f);
    const Vec3 verts[4] = {
        Vec3( s,  s,  s),
        Vec3(-s, -s,  s),
        Vec3(-s,  s, -s),
        Vec3( s, -s, -s)
    };

    float b[4];
    if (!tetra_barycentric(p, verts, b)) return false;

    const float eps = 1e-6f;
    for (int i = 0; i < 4; ++i) {
        if (b[i] < -eps || b[i] > 1.0f + eps) return false;
    }

    depth = std::max(1, depth);
    for (int iter = 0; iter < depth - 1; ++iter) {
        int pivot = 0;
        for (int i = 1; i < 4; ++i) {
            if (b[i] > b[pivot]) pivot = i;
        }
        if (b[pivot] < 0.5f - eps) return false;

        float next[4];
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

template <typename inside_fn_t>
static void build_voxel_surface(object_t *obj, int grid, const Vec3 &bmin, const Vec3 &bmax, inside_fn_t inside_fn)
{
    if (!obj || grid < 2) return;

    const float step_x = (bmax.x - bmin.x) / (float)grid;
    const float step_y = (bmax.y - bmin.y) / (float)grid;
    const float step_z = (bmax.z - bmin.z) / (float)grid;
    const float step = std::min(step_x, std::min(step_y, step_z));
    if (step <= 0.0f) return;

    std::vector<ivec3_t> cells;
    std::set<std::string> occupied;

    for (int iz = 0; iz < grid; ++iz) {
        const float z = bmin.z + ((float)iz + 0.5f) * step;
        for (int iy = 0; iy < grid; ++iy) {
            const float y = bmin.y + ((float)iy + 0.5f) * step;
            for (int ix = 0; ix < grid; ++ix) {
                const float x = bmin.x + ((float)ix + 0.5f) * step;
                if (!inside_fn(Vec3(x, y, z))) continue;
                cells.push_back(ivec3_t{ix, iy, iz});
                occupied.insert(voxel_key(ix, iy, iz));
            }
        }
    }

    if (cells.empty()) return;

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    for (size_t i = 0; i < cells.size(); ++i) {
        const ivec3_t c = cells[i];
        const Vec3 center(
            bmin.x + ((float)c.x + 0.5f) * step,
            bmin.y + ((float)c.y + 0.5f) * step,
            bmin.z + ((float)c.z + 0.5f) * step
        );
        add_cube_faces(obj, out, center, step, occupied, c.x, c.y, c.z);
    }
}

static void build_revolution_band(
    object_t *obj, shape_t &shape,
    size_t seg_u, size_t seg_v,
    const std::vector<Vec3> &points,
    const std::vector<Vec3> &normals
)
{
    const int stride = (int)(seg_v + 1);
    for (size_t iu = 0; iu <= seg_u; ++iu) {
        const float u = (float)iu / (float)seg_u;
        const float a = (float)(nmath::PI_DOUBLE * 2.0) * u;
        const float ca = nmath_cos(a);
        const float sa = nmath_sin(a);

        for (size_t iv = 0; iv <= seg_v; ++iv) {
            const Vec3 p2 = points[iv];
            const Vec3 n2 = normals[iv];
            const Vec3 p(p2.x * ca, p2.y, p2.x * sa);
            Vec3 n(n2.x * ca, n2.y, n2.x * sa);
            if (n.length() <= 1e-8f) n = Vec3(ca, 0, sa);
            n.normalize();
            uv_t uv;
            uv.u = u;
            uv.v = (float)iv / (float)seg_v;
            append_vertex(obj, p, n, &uv);
        }
    }

    for (size_t iu = 0; iu < seg_u; ++iu) {
        for (size_t iv = 0; iv < seg_v; ++iv) {
            const int i0 = (int)(iu * (seg_v + 1) + iv);
            const int i1 = i0 + 1;
            const int i2 = i0 + stride + 1;
            const int i3 = i0 + stride;
            append_quad(shape, i0, i3, i2, i1, true);
        }
    }
}

} /* namespace */

void cylinder(object_t *obj, size_t resolution)
{
    if (!obj) return;
    const size_t seg = std::max((size_t)16, resolution);

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    const float r = 0.5f;
    const float h0 = -0.5f;
    const float h1 = 0.5f;

    for (size_t i = 0; i <= seg; ++i) {
        const float u = (float)i / (float)seg;
        const float a = (float)(nmath::PI_DOUBLE * 2.0) * u;
        const float ca = nmath_cos(a);
        const float sa = nmath_sin(a);
        const Vec3 n(ca, 0.0f, sa);

        uv_t uv0 = {u, 0.0f};
        uv_t uv1 = {u, 1.0f};
        append_vertex(obj, Vec3(r * ca, h0, r * sa), n, &uv0);
        append_vertex(obj, Vec3(r * ca, h1, r * sa), n, &uv1);
    }

    for (size_t i = 0; i < seg; ++i) {
        const int i0 = (int)(i * 2);
        const int i1 = i0 + 1;
        const int i2 = i0 + 3;
        const int i3 = i0 + 2;
        append_quad(out, i0, i3, i2, i1, true);
    }
}

void capped_cylinder(object_t *obj, size_t resolution)
{
    cylinder(obj, resolution);
    if (!obj || obj->shapes.empty()) return;

    shape_t &out = obj->shapes.back();
    const size_t seg = std::max((size_t)16, resolution);
    const float r = 0.5f;
    const float h0 = -0.5f;
    const float h1 = 0.5f;

    const int top_center = append_vertex(obj, Vec3(0, h1, 0), Vec3(0, 1, 0));
    const int bot_center = append_vertex(obj, Vec3(0, h0, 0), Vec3(0, -1, 0));

    std::vector<int> top_ring;
    std::vector<int> bot_ring;
    top_ring.reserve(seg + 1);
    bot_ring.reserve(seg + 1);

    for (size_t i = 0; i <= seg; ++i) {
        const float u = (float)i / (float)seg;
        const float a = (float)(nmath::PI_DOUBLE * 2.0) * u;
        const float ca = nmath_cos(a);
        const float sa = nmath_sin(a);
        top_ring.push_back(append_vertex(obj, Vec3(r * ca, h1, r * sa), Vec3(0, 1, 0)));
        bot_ring.push_back(append_vertex(obj, Vec3(r * ca, h0, r * sa), Vec3(0, -1, 0)));
    }

    for (size_t i = 0; i < seg; ++i) {
        append_triangle(out, top_center, top_ring[i], top_ring[i + 1], false);
        append_triangle(out, bot_center, bot_ring[i + 1], bot_ring[i], false);
    }
}

void cone(object_t *obj, size_t resolution)
{
    if (!obj) return;
    const size_t seg = std::max((size_t)16, resolution);
    const float r0 = 0.5f;
    const float r1 = 0.0f;
    const float h0 = -0.5f;
    const float h1 = 0.5f;

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    for (size_t i = 0; i <= seg; ++i) {
        const float u = (float)i / (float)seg;
        const float a = (float)(nmath::PI_DOUBLE * 2.0) * u;
        const float ca = nmath_cos(a);
        const float sa = nmath_sin(a);
        Vec3 n(ca, r0 - r1, sa);
        n.normalize();
        uv_t uv0 = {u, 0.0f};
        uv_t uv1 = {u, 1.0f};
        append_vertex(obj, Vec3(r0 * ca, h0, r0 * sa), n, &uv0);
        append_vertex(obj, Vec3(r1 * ca, h1, r1 * sa), n, &uv1);
    }

    for (size_t i = 0; i < seg; ++i) {
        const int i0 = (int)(i * 2);
        const int i1 = i0 + 1;
        const int i2 = i0 + 3;
        const int i3 = i0 + 2;
        append_quad(out, i0, i3, i2, i1, true);
    }

    const int center = append_vertex(obj, Vec3(0, h0, 0), Vec3(0, -1, 0));
    std::vector<int> ring;
    ring.reserve(seg + 1);
    for (size_t i = 0; i <= seg; ++i) {
        const float u = (float)i / (float)seg;
        const float a = (float)(nmath::PI_DOUBLE * 2.0) * u;
        ring.push_back(append_vertex(obj, Vec3(r0 * nmath_cos(a), h0, r0 * nmath_sin(a)), Vec3(0, -1, 0)));
    }
    for (size_t i = 0; i < seg; ++i) append_triangle(out, center, ring[i + 1], ring[i], false);
}

void truncated_cone(object_t *obj, size_t resolution)
{
    if (!obj) return;
    const size_t seg = std::max((size_t)16, resolution);
    const float r0 = 0.55f;
    const float r1 = 0.25f;
    const float h0 = -0.5f;
    const float h1 = 0.5f;

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    for (size_t i = 0; i <= seg; ++i) {
        const float u = (float)i / (float)seg;
        const float a = (float)(nmath::PI_DOUBLE * 2.0) * u;
        const float ca = nmath_cos(a);
        const float sa = nmath_sin(a);
        Vec3 n(ca, r0 - r1, sa);
        n.normalize();
        uv_t uv0 = {u, 0.0f};
        uv_t uv1 = {u, 1.0f};
        append_vertex(obj, Vec3(r0 * ca, h0, r0 * sa), n, &uv0);
        append_vertex(obj, Vec3(r1 * ca, h1, r1 * sa), n, &uv1);
    }

    for (size_t i = 0; i < seg; ++i) {
        const int i0 = (int)(i * 2);
        const int i1 = i0 + 1;
        const int i2 = i0 + 3;
        const int i3 = i0 + 2;
        append_quad(out, i0, i3, i2, i1, true);
    }

    const int ctop = append_vertex(obj, Vec3(0, h1, 0), Vec3(0, 1, 0));
    const int cbot = append_vertex(obj, Vec3(0, h0, 0), Vec3(0, -1, 0));
    std::vector<int> top;
    std::vector<int> bot;
    for (size_t i = 0; i <= seg; ++i) {
        const float u = (float)i / (float)seg;
        const float a = (float)(nmath::PI_DOUBLE * 2.0) * u;
        top.push_back(append_vertex(obj, Vec3(r1 * nmath_cos(a), h1, r1 * nmath_sin(a)), Vec3(0, 1, 0)));
        bot.push_back(append_vertex(obj, Vec3(r0 * nmath_cos(a), h0, r0 * nmath_sin(a)), Vec3(0, -1, 0)));
    }
    for (size_t i = 0; i < seg; ++i) {
        append_triangle(out, ctop, top[i], top[i + 1], false);
        append_triangle(out, cbot, bot[i + 1], bot[i], false);
    }
}

void capsule(object_t *obj, size_t resolution)
{
    if (!obj) return;
    const size_t seg_u = std::max((size_t)20, resolution);
    const size_t seg_v = std::max((size_t)12, resolution / 2);

    const float r = 0.35f;
    const float h = 0.45f;

    std::vector<Vec3> profile_p;
    std::vector<Vec3> profile_n;
    profile_p.reserve(seg_v + 1);
    profile_n.reserve(seg_v + 1);

    for (size_t i = 0; i <= seg_v; ++i) {
        const float t = (float)i / (float)seg_v; // [0,1]
        const float y = (-h - r) + t * (2.0f * (h + r));

        float x = r;
        float ny = 0.0f;
        if (y < -h) {
            const float dy = y + h;
            x = nmath_sqrt(std::max(0.0f, r * r - dy * dy));
            ny = dy / r;
        } else if (y > h) {
            const float dy = y - h;
            x = nmath_sqrt(std::max(0.0f, r * r - dy * dy));
            ny = dy / r;
        }

        float nx = (x > 1e-8f) ? x / r : 0.0f;
        Vec3 n(nx, ny, 0.0f);
        if (n.length() <= 1e-8f) n = Vec3(1, 0, 0);
        n.normalize();

        profile_p.push_back(Vec3(x, y, 0.0f));
        profile_n.push_back(n);
    }

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();
    build_revolution_band(obj, out, seg_u, seg_v, profile_p, profile_n);
}

void torus_knot(object_t *obj, size_t resolution)
{
    if (!obj) return;
    const size_t seg_u = std::max((size_t)72, resolution * 2);
    const size_t seg_v = std::max((size_t)10, resolution / 3);

    const int q = 3;
    const float scale = 0.9f;
    const float tube_r = 0.16f;

    std::vector<Vec3> centers(seg_u);
    std::vector<Vec3> tangents(seg_u);
    std::vector<Vec3> normals(seg_u);
    std::vector<Vec3> binormals(seg_u);

    for (size_t i = 0; i < seg_u; ++i) {
        const float u = (float)i / (float)seg_u;
        const float t = (float)(nmath::PI_DOUBLE * 2.0) * u;
        const float ct = nmath_cos(t);
        const float st = nmath_sin(t);
        const float cqt = nmath_cos((float)q * t);
        const float sqt = nmath_sin((float)q * t);
        const float radial = 1.0f + 0.4f * cqt;
        centers[i] = Vec3(scale * radial * ct, scale * 0.4f * sqt, scale * radial * st);
    }

    for (size_t i = 0; i < seg_u; ++i) {
        const size_t ip = (i + 1) % seg_u;
        const size_t im = (i + seg_u - 1) % seg_u;
        Vec3 t = centers[ip] - centers[im];
        if (t.length() <= 1e-8f) t = Vec3(1, 0, 0);
        t.normalize();
        tangents[i] = t;
    }

    Vec3 prev_n(0, 1, 0);
    for (size_t i = 0; i < seg_u; ++i) {
        Vec3 n = nmath::cross(prev_n, tangents[i]);
        if (n.length() <= 1e-8f) n = nmath::cross(Vec3(0, 1, 0), tangents[i]);
        if (n.length() <= 1e-8f) n = nmath::cross(Vec3(1, 0, 0), tangents[i]);
        n.normalize();
        Vec3 b = nmath::cross(tangents[i], n);
        b.normalize();
        n = nmath::cross(b, tangents[i]).normalized();
        normals[i] = n;
        binormals[i] = b;
        prev_n = n;
    }

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    for (size_t i = 0; i <= seg_u; ++i) {
        const size_t ii = i % seg_u;
        const float u = (float)i / (float)seg_u;
        for (size_t j = 0; j <= seg_v; ++j) {
            const float v = (float)j / (float)seg_v;
            const float a = (float)(nmath::PI_DOUBLE * 2.0) * v;
            const float ca = nmath_cos(a);
            const float sa = nmath_sin(a);
            Vec3 dir = (normals[ii] * ca + binormals[ii] * sa).normalized();
            Vec3 p3 = centers[ii] + dir * tube_r;
            uv_t uv = {u, v};
            append_vertex(obj, p3, dir, &uv);
        }
    }

    const int stride = (int)(seg_v + 1);
    for (size_t i = 0; i < seg_u; ++i) {
        for (size_t j = 0; j < seg_v; ++j) {
            const int i0 = (int)(i * (seg_v + 1) + j);
            const int i1 = i0 + 1;
            const int i2 = i0 + stride + 1;
            const int i3 = i0 + stride;
            append_quad(out, i0, i3, i2, i1, true);
        }
    }
}

void icosphere(object_t *obj, size_t resolution)
{
    if (!obj) return;
    const int iters = clampi((int)(resolution / 16), 0, 3);

    std::vector<Vec3> verts;
    std::vector<tri_t> faces;
    build_icosphere_data(verts, faces, iters);

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    std::vector<int> remap(verts.size(), -1);
    for (size_t i = 0; i < verts.size(); ++i) remap[i] = append_vertex(obj, verts[i], verts[i], 0);
    for (size_t i = 0; i < faces.size(); ++i) append_triangle(out, remap[faces[i].a], remap[faces[i].b], remap[faces[i].c], false);
}

void geodesic_dome(object_t *obj, size_t resolution)
{
    if (!obj) return;
    const int iters = clampi((int)(resolution / 16), 0, 3);

    std::vector<Vec3> verts;
    std::vector<tri_t> faces;
    build_icosphere_data(verts, faces, iters);

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    std::vector<int> remap(verts.size(), -1);
    for (size_t i = 0; i < faces.size(); ++i) {
        const tri_t t = faces[i];
        const Vec3 a = verts[t.a];
        const Vec3 b = verts[t.b];
        const Vec3 c = verts[t.c];
        if (a.y < -1e-5f || b.y < -1e-5f || c.y < -1e-5f) continue;

        if (remap[t.a] < 0) remap[t.a] = append_vertex(obj, a, a, 0);
        if (remap[t.b] < 0) remap[t.b] = append_vertex(obj, b, b, 0);
        if (remap[t.c] < 0) remap[t.c] = append_vertex(obj, c, c, 0);
        append_triangle(out, remap[t.a], remap[t.b], remap[t.c], false);
    }
}

void icosa_cage(object_t *obj, size_t resolution)
{
    if (!obj) return;

    // Keep tessellation bounded: this generator creates prism struts per edge.
    const int iters = clampi((int)(resolution / 12), 0, 3);

    std::vector<Vec3> verts;
    std::vector<tri_t> faces;
    build_icosphere_data(verts, faces, iters);

    std::set<std::pair<int, int> > edges;
    for (size_t i = 0; i < faces.size(); ++i) {
        const tri_t &t = faces[i];
        const int ab0 = std::min(t.a, t.b);
        const int ab1 = std::max(t.a, t.b);
        const int bc0 = std::min(t.b, t.c);
        const int bc1 = std::max(t.b, t.c);
        const int ca0 = std::min(t.c, t.a);
        const int ca1 = std::max(t.c, t.a);
        edges.insert(std::make_pair(ab0, ab1));
        edges.insert(std::make_pair(bc0, bc1));
        edges.insert(std::make_pair(ca0, ca1));
    }

    if (edges.empty()) return;

    float avg_len = 0.0f;
    for (std::set<std::pair<int, int> >::const_iterator it = edges.begin(); it != edges.end(); ++it) {
        avg_len += (verts[it->second] - verts[it->first]).length();
    }
    avg_len /= (float)edges.size();
    const float strut_radius = std::max(0.01f, avg_len * 0.16f);

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    const float c60 = 0.5f;
    const float s60 = 0.8660254f;

    for (std::set<std::pair<int, int> >::const_iterator it = edges.begin(); it != edges.end(); ++it) {
        const Vec3 pa = verts[it->first];
        const Vec3 pb = verts[it->second];
        Vec3 axis = pb - pa;
        if (axis.length() <= 1e-6f) continue;
        axis.normalize();

        Vec3 radial = (pa + pb) * 0.5f;
        if (radial.length() <= 1e-6f) radial = Vec3(0, 1, 0);
        radial.normalize();

        Vec3 u = nmath::cross(axis, radial);
        if (u.length() <= 1e-6f) u = nmath::cross(axis, Vec3(0, 1, 0));
        if (u.length() <= 1e-6f) u = nmath::cross(axis, Vec3(1, 0, 0));
        if (u.length() <= 1e-6f) continue;
        u.normalize();
        Vec3 v = nmath::cross(axis, u).normalized();

        Vec3 dirs[3];
        dirs[0] = u;
        dirs[1] = (u * -c60 + v * s60).normalized();
        dirs[2] = (u * -c60 - v * s60).normalized();

        int ia[3];
        int ib[3];
        for (int k = 0; k < 3; ++k) {
            ia[k] = append_vertex(obj, pa + dirs[k] * strut_radius, dirs[k], 0);
            ib[k] = append_vertex(obj, pb + dirs[k] * strut_radius, dirs[k], 0);
        }

        for (int k = 0; k < 3; ++k) {
            const int kn = (k + 1) % 3;
            append_triangle(out, ia[k], ib[k], ib[kn], false);
            append_triangle(out, ia[k], ib[kn], ia[kn], false);
        }
    }
}

void sierpinski_tetrahedron(object_t *obj, size_t resolution)
{
    if (!obj) return;
    const int depth = clampi((int)resolution, 1, 4);

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    const float s = 1.0f / nmath_sqrt(3.0f);
    // Normalize canonical bounds to unit size [-0.5, 0.5].
    const float unit_scale = 0.5f / s;
    std::vector<std::vector<Vec3> > tets;
    tets.push_back(std::vector<Vec3>{
        Vec3( s,  s,  s) * unit_scale,
        Vec3(-s, -s,  s) * unit_scale,
        Vec3(-s,  s, -s) * unit_scale,
        Vec3( s, -s, -s) * unit_scale
    });

    for (int d = 1; d < depth; ++d) {
        std::vector<std::vector<Vec3> > next;
        for (size_t i = 0; i < tets.size(); ++i) {
            const std::vector<Vec3> &t = tets[i];
            const Vec3 m01 = (t[0] + t[1]) * 0.5f;
            const Vec3 m02 = (t[0] + t[2]) * 0.5f;
            const Vec3 m03 = (t[0] + t[3]) * 0.5f;
            const Vec3 m12 = (t[1] + t[2]) * 0.5f;
            const Vec3 m13 = (t[1] + t[3]) * 0.5f;
            const Vec3 m23 = (t[2] + t[3]) * 0.5f;
            next.push_back(std::vector<Vec3>{t[0], m01, m02, m03});
            next.push_back(std::vector<Vec3>{m01, t[1], m12, m13});
            next.push_back(std::vector<Vec3>{m02, m12, t[2], m23});
            next.push_back(std::vector<Vec3>{m03, m13, m23, t[3]});
        }
        tets.swap(next);
    }

    const auto add_face_outward = [&](const std::vector<Vec3> &t, int ia, int ib, int ic) {
        const Vec3 a = t[(size_t)ia];
        const Vec3 b = t[(size_t)ib];
        const Vec3 c = t[(size_t)ic];
        const Vec3 center = (t[0] + t[1] + t[2] + t[3]) * 0.25f;
        Vec3 n = nmath::cross(b - a, c - a);
        if (n.length() <= 1e-8f) return;
        const Vec3 face_center = (a + b + c) * (1.0f / 3.0f);
        if (nmath::dot(n, face_center - center) < 0.0f) {
            add_triangle_flat(obj, out, a, c, b);
        } else {
            add_triangle_flat(obj, out, a, b, c);
        }
    };

    for (size_t i = 0; i < tets.size(); ++i) {
        const std::vector<Vec3> &t = tets[i];
        add_face_outward(t, 0, 1, 2);
        add_face_outward(t, 0, 3, 1);
        add_face_outward(t, 0, 2, 3);
        add_face_outward(t, 1, 3, 2);
    }
}

void menger_sponge(object_t *obj, size_t resolution)
{
    if (!obj) return;
    const int depth = clampi((int)resolution, 1, 3);

    typedef struct {
        int x;
        int y;
        int z;
        float size;
    } cube_t;

    std::vector<cube_t> cubes;
    cubes.push_back(cube_t{0, 0, 0, 1.0f});

    for (int d = 0; d < depth; ++d) {
        std::vector<cube_t> next;
        for (size_t i = 0; i < cubes.size(); ++i) {
            const cube_t c = cubes[i];
            const float cs = c.size / 3.0f;
            for (int ix = -1; ix <= 1; ++ix) {
                for (int iy = -1; iy <= 1; ++iy) {
                    for (int iz = -1; iz <= 1; ++iz) {
                        int zeros = (ix == 0) + (iy == 0) + (iz == 0);
                        if (zeros >= 2) continue;
                        next.push_back(cube_t{c.x * 3 + ix, c.y * 3 + iy, c.z * 3 + iz, cs});
                    }
                }
            }
        }
        cubes.swap(next);
    }

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    std::set<std::string> occupied;
    for (size_t i = 0; i < cubes.size(); ++i) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%d,%d,%d", cubes[i].x, cubes[i].y, cubes[i].z);
        occupied.insert(std::string(buf));
    }

    // Normalize generated Menger mesh to unit canonical bounds [-0.5, 0.5].
    const float norm = 1.0f;
    for (size_t i = 0; i < cubes.size(); ++i) {
        const cube_t c = cubes[i];
        const Vec3 center(
            (float)c.x * c.size / norm,
            (float)c.y * c.size / norm,
            (float)c.z * c.size / norm
        );
        add_cube_faces(obj, out, center, c.size / norm, occupied, c.x, c.y, c.z);
    }
}

void menger_sponge_implicit(object_t *obj, size_t resolution)
{
    if (!obj) return;
    const int depth = clampi((int)resolution, 1, 4);
    const int grid = 6 * ipowi(3, depth - 1);
    // Normalize generated implicit Menger mesh to unit canonical bounds [-0.5, 0.5].
    const float extent = 0.5f;

    build_voxel_surface(obj, grid, Vec3(-extent, -extent, -extent), Vec3(extent, extent, extent),
                        [depth](const Vec3 &p) -> bool {
                            return inside_menger_local(p * 2.0f, depth);
                        });
}

void sierpinski_tetrahedron_implicit(object_t *obj, size_t resolution)
{
    if (!obj) return;
    const int depth = clampi((int)resolution, 1, 4);
    const int grid = 16 << (depth - 1);
    const float s = 1.0f / nmath_sqrt(3.0f);
    // Normalize canonical bounds to unit size [-0.5, 0.5].
    const float extent = 0.5f;
    const float local_scale = s / extent;

    build_voxel_surface(obj, grid, Vec3(-extent, -extent, -extent), Vec3(extent, extent, extent),
                        [depth, local_scale](const Vec3 &p) -> bool {
                            return inside_sierpinski_tetra_local(p * local_scale, depth);
                        });
}

void mobius_strip(object_t *obj, size_t resolution, float radius, float width)
{
    if (!obj) return;
    const size_t seg_u = std::max((size_t)64, resolution);
    const size_t seg_v = std::max((size_t)12, resolution / 4);
    const float R = std::max(0.05f, radius);
    const float W = std::max(0.01f, width * 0.5f);

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    for (size_t iu = 0; iu <= seg_u; ++iu) {
        const float u = (float)iu / (float)seg_u;
        const float a = (float)(nmath::PI_DOUBLE * 2.0) * u;

        for (size_t iv = 0; iv <= seg_v; ++iv) {
            const float v = (float)iv / (float)seg_v;
            const float w = (v - 0.5f) * 2.0f * W;

            const float ca = nmath_cos(a);
            const float sa = nmath_sin(a);
            const float c2 = nmath_cos(0.5f * a);
            const float s2 = nmath_sin(0.5f * a);

            const Vec3 p((R + w * c2) * ca, w * s2, (R + w * c2) * sa);

            const float eps = 0.001f;
            const float ap = a + eps;
            const float cp = nmath_cos(ap);
            const float sp = nmath_sin(ap);
            const float c2p = nmath_cos(0.5f * ap);
            const float s2p = nmath_sin(0.5f * ap);
            const Vec3 pu((R + w * c2p) * cp, w * s2p, (R + w * c2p) * sp);
            const float wp = w + eps;
            const Vec3 pv((R + wp * c2) * ca, wp * s2, (R + wp * c2) * sa);
            Vec3 n = nmath::cross(pu - p, pv - p);
            if (n.length() <= 1e-8f) n = Vec3(0, 1, 0);
            n.normalize();

            uv_t uv = {u, v};
            append_vertex(obj, p, n, &uv);
        }
    }

    const int stride = (int)(seg_v + 1);
    for (size_t iu = 0; iu < seg_u; ++iu) {
        for (size_t iv = 0; iv < seg_v; ++iv) {
            const int i0 = (int)(iu * (seg_v + 1) + iv);
            const int i1 = i0 + 1;
            const int i2 = i0 + stride + 1;
            const int i3 = i0 + stride;
            append_quad(out, i0, i3, i2, i1, true);
        }
    }
}

void klein_bottle(object_t *obj, size_t resolution)
{
    if (!obj) return;
    const size_t seg_u = std::max((size_t)48, resolution);
    const size_t seg_v = std::max((size_t)24, resolution / 2);
    const float r = 2.0f;
    const float s = 0.18f;

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    auto eval = [&](float u, float v) -> Vec3 {
        const float su = nmath_sin(u);
        const float cu = nmath_cos(u);
        const float s2u = nmath_sin(0.5f * u);
        const float c2u = nmath_cos(0.5f * u);
        const float sv = nmath_sin(v);
        const float s2v = nmath_sin(2.0f * v);
        const float radial = r + c2u * sv - s2u * s2v;
        return Vec3(s * radial * cu, s * (s2u * sv + c2u * s2v), s * radial * su);
    };

    for (size_t iu = 0; iu <= seg_u; ++iu) {
        const float u = (float)iu / (float)seg_u;
        const float a = (float)(nmath::PI_DOUBLE * 2.0) * u;
        for (size_t iv = 0; iv <= seg_v; ++iv) {
            const float v = (float)iv / (float)seg_v;
            const float b = (float)(nmath::PI_DOUBLE * 2.0) * v;
            const Vec3 p = eval(a, b);
            const Vec3 pu = eval(a + 0.001f, b);
            const Vec3 pv = eval(a, b + 0.001f);
            Vec3 n = nmath::cross(pu - p, pv - p);
            if (n.length() <= 1e-8f) n = Vec3(0, 1, 0);
            n.normalize();
            uv_t uv = {u, v};
            append_vertex(obj, p, n, &uv);
        }
    }

    const int stride = (int)(seg_v + 1);
    for (size_t iu = 0; iu < seg_u; ++iu) {
        for (size_t iv = 0; iv < seg_v; ++iv) {
            const int i0 = (int)(iu * (seg_v + 1) + iv);
            const int i1 = i0 + 1;
            const int i2 = i0 + stride + 1;
            const int i3 = i0 + stride;
            append_quad(out, i0, i3, i2, i1, true);
        }
    }
}

namespace {

struct rng_t {
    uint32_t state;
};

static uint32_t rng_next_u32(rng_t &rng)
{
    uint32_t x = rng.state;
    if (x == 0u) x = 0x9e3779b9u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng.state = x;
    return x;
}

static float rng_next_unit(rng_t &rng)
{
    return (float)((rng_next_u32(rng) & 0x00ffffffu) / 16777215.0);
}

static Vec3 random_unit_vec3(rng_t &rng)
{
    const float z = rng_next_unit(rng) * 2.0f - 1.0f;
    const float a = (float)(nmath::PI_DOUBLE * 2.0) * rng_next_unit(rng);
    const float r = nmath_sqrt(std::max(0.0f, 1.0f - z * z));
    return Vec3(r * nmath_cos(a), z, r * nmath_sin(a));
}

static float fractf(float x)
{
    return x - std::floor(x);
}

static float hash_noise3(const Vec3 &p, uint32_t seed)
{
    const float s = (float)(seed & 0xffffu) * 0.013517f;
    const float v = p.x * 12.9898f + p.y * 78.233f + p.z * 37.719f + s;
    return fractf(nmath_sin(v) * 43758.5453f) * 2.0f - 1.0f;
}

static float hash_noise2(float x, float y, uint32_t seed)
{
    const float s = (float)(seed & 0xffffu) * 0.017113f;
    const float v = x * 127.1f + y * 311.7f + s;
    return fractf(nmath_sin(v) * 43758.5453f) * 2.0f - 1.0f;
}

static float smoothstepf(float t)
{
    t = std::max(0.0f, std::min(1.0f, t));
    return t * t * (3.0f - 2.0f * t);
}

static float lerpf(float a, float b, float t)
{
    return a + (b - a) * t;
}

static float value_noise2(float x, float y, uint32_t seed)
{
    const float xf = std::floor(x);
    const float yf = std::floor(y);
    const float tx = smoothstepf(x - xf);
    const float ty = smoothstepf(y - yf);

    const float n00 = hash_noise2(xf, yf, seed);
    const float n10 = hash_noise2(xf + 1.0f, yf, seed);
    const float n01 = hash_noise2(xf, yf + 1.0f, seed);
    const float n11 = hash_noise2(xf + 1.0f, yf + 1.0f, seed);

    const float nx0 = lerpf(n00, n10, tx);
    const float nx1 = lerpf(n01, n11, tx);
    return lerpf(nx0, nx1, ty);
}

static float fbm_noise(const Vec3 &p, uint32_t seed, size_t octaves)
{
    float sum = 0.0f;
    float amp = 1.0f;
    float f = 1.0f;
    float norm = 0.0f;
    for (size_t i = 0; i < octaves; ++i) {
        const Vec3 pp = p * f + Vec3((float)i * 1.37f, (float)i * 2.11f, (float)i * 0.73f);
        sum += amp * hash_noise3(pp, seed + (uint32_t)(i * 1664525u));
        norm += amp;
        amp *= 0.5f;
        f *= 2.03f;
    }
    if (norm <= 1e-8f) return 0.0f;
    return sum / norm;
}

static float fbm_noise2_adv(float x, float y, uint32_t seed, size_t octaves, float lacunarity, float gain)
{
    float sum = 0.0f;
    float amp = 1.0f;
    float f = 1.0f;
    float norm = 0.0f;
    lacunarity = std::max(1.01f, lacunarity);
    gain = std::max(0.05f, std::min(0.95f, gain));

    for (size_t i = 0; i < octaves; ++i) {
        const float ox = (float)i * 13.17f;
        const float oy = (float)i * 7.31f;
        sum += amp * value_noise2(x * f + ox, y * f + oy, seed + (uint32_t)(i * 1664525u));
        norm += amp;
        amp *= gain;
        f *= lacunarity;
    }

    if (norm <= 1e-8f) return 0.0f;
    return sum / norm;
}

static float ridged_noise2_adv(float x, float y, uint32_t seed, size_t octaves, float lacunarity, float gain)
{
    float sum = 0.0f;
    float amp = 0.5f;
    float f = 1.0f;
    float norm = 0.0f;
    lacunarity = std::max(1.01f, lacunarity);
    gain = std::max(0.05f, std::min(0.95f, gain));

    for (size_t i = 0; i < octaves; ++i) {
        const float n = value_noise2(x * f + (float)i * 5.13f, y * f + (float)i * 9.27f, seed + (uint32_t)(911382323u + i * 2654435761u));
        const float r = 1.0f - std::fabs(n);
        sum += amp * (r * r);
        norm += amp;
        amp *= gain;
        f *= lacunarity;
    }

    if (norm <= 1e-8f) return 0.0f;
    return sum / norm;
}

static void append_torus_link(object_t *obj, shape_t &shape, const Vec3 &center, const Vec3 &axis,
                              float major_radius, float minor_radius, size_t seg_u, size_t seg_v)
{
    if (!obj) return;
    if (major_radius <= 0.0f || minor_radius <= 0.0f) return;
    seg_u = std::max((size_t)24, seg_u);
    seg_v = std::max((size_t)12, seg_v);

    Vec3 n = axis;
    if (n.length() <= 1e-8f) n = Vec3(0, 1, 0);
    n.normalize();
    Vec3 u = nmath::cross(n, Vec3(0, 1, 0));
    if (u.length() <= 1e-8f) u = nmath::cross(n, Vec3(1, 0, 0));
    if (u.length() <= 1e-8f) return;
    u.normalize();
    Vec3 v = nmath::cross(n, u).normalized();

    const int base_index = (int)(obj->attributes.v.size() / 3);
    for (size_t iu = 0; iu <= seg_u; ++iu) {
        const float tu = (float)iu / (float)seg_u;
        const float au = (float)(nmath::PI_DOUBLE * 2.0) * tu;
        const float cu = nmath_cos(au);
        const float su = nmath_sin(au);
        const Vec3 radial = (u * cu + v * su).normalized();
        const Vec3 ring_center = center + radial * major_radius;

        for (size_t iv = 0; iv <= seg_v; ++iv) {
            const float tv = (float)iv / (float)seg_v;
            const float av = (float)(nmath::PI_DOUBLE * 2.0) * tv;
            const float cv = nmath_cos(av);
            const float sv = nmath_sin(av);
            Vec3 normal = (radial * cv + n * sv);
            if (normal.length() <= 1e-8f) normal = radial;
            normal.normalize();
            const Vec3 pos = ring_center + normal * minor_radius;
            uv_t uv = {tu, tv};
            append_vertex(obj, pos, normal, &uv);
        }
    }

    const int stride = (int)(seg_v + 1);
    for (size_t iu = 0; iu < seg_u; ++iu) {
        for (size_t iv = 0; iv < seg_v; ++iv) {
            const int i0 = base_index + (int)(iu * (seg_v + 1) + iv);
            const int i1 = i0 + 1;
            const int i2 = i0 + stride + 1;
            const int i3 = i0 + stride;
            append_quad(shape, i0, i3, i2, i1, true);
        }
    }
}

static Vec3 safe_normalized(const Vec3 &v, const Vec3 &fallback)
{
    Vec3 out = v;
    if (out.length() <= 1e-8f) return fallback;
    out.normalize();
    return out;
}

static void build_polyline_lengths(const std::vector<Vec3> &pts, std::vector<float> &cum_lengths)
{
    cum_lengths.clear();
    if (pts.empty()) return;
    cum_lengths.resize(pts.size(), 0.0f);
    for (size_t i = 1; i < pts.size(); ++i) {
        cum_lengths[i] = cum_lengths[i - 1] + (pts[i] - pts[i - 1]).length();
    }
}

static Vec3 sample_polyline(const std::vector<Vec3> &pts, const std::vector<float> &cum_lengths, float s)
{
    if (pts.empty()) return Vec3(0, 0, 0);
    if (pts.size() == 1) return pts[0];

    const float total = cum_lengths.back();
    if (s <= 0.0f) return pts.front();
    if (s >= total) return pts.back();

    size_t seg = 1;
    while (seg < cum_lengths.size() && cum_lengths[seg] < s) ++seg;
    if (seg >= pts.size()) return pts.back();

    const float a = cum_lengths[seg - 1];
    const float b = cum_lengths[seg];
    const float den = std::max(1e-8f, b - a);
    const float t = (s - a) / den;
    return pts[seg - 1] * (1.0f - t) + pts[seg] * t;
}

} /* namespace */

void hairball(object_t *obj, size_t resolution, int seed, float radius, size_t fibers)
{
    if (!obj) return;
    if (radius <= 0.0f) radius = 1.0f;

    const int iters = clampi((int)(resolution / 16), 0, 3);
    size_t fiber_count = fibers;
    if (fiber_count == 0) fiber_count = std::max((size_t)64, resolution * 10);
    fiber_count = std::min((size_t)20000, std::max((size_t)16, fiber_count));

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    std::vector<Vec3> sphere_verts;
    std::vector<tri_t> sphere_faces;
    build_icosphere_data(sphere_verts, sphere_faces, iters);
    const float core_radius = radius * 0.72f;

    std::vector<int> remap(sphere_verts.size(), -1);
    for (size_t i = 0; i < sphere_verts.size(); ++i) {
        const Vec3 n = sphere_verts[i].normalized();
        remap[i] = append_vertex(obj, n * core_radius, n, 0);
    }
    for (size_t i = 0; i < sphere_faces.size(); ++i) {
        const tri_t &t = sphere_faces[i];
        append_triangle(out, remap[t.a], remap[t.b], remap[t.c], false);
    }

    rng_t rng;
    rng.state = (uint32_t)seed ^ 0xa3c59ac3u;
    if (rng.state == 0u) rng.state = 1u;

    for (size_t i = 0; i < fiber_count; ++i) {
        const Vec3 dir = random_unit_vec3(rng);
        const Vec3 base = dir * core_radius;

        Vec3 tangent0 = nmath::cross(dir, Vec3(0, 1, 0));
        if (tangent0.length() <= 1e-6f) tangent0 = nmath::cross(dir, Vec3(1, 0, 0));
        if (tangent0.length() <= 1e-6f) continue;
        tangent0.normalize();
        Vec3 tangent1 = nmath::cross(dir, tangent0).normalized();

        const float twist = (float)(nmath::PI_DOUBLE * 2.0) * rng_next_unit(rng);
        const float c = nmath_cos(twist);
        const float s = nmath_sin(twist);
        Vec3 t0 = (tangent0 * c + tangent1 * s).normalized();
        Vec3 t1 = nmath::cross(dir, t0).normalized();

        const float base_r = radius * (0.006f + 0.02f * rng_next_unit(rng));
        const float len = radius * (0.12f + 0.38f * rng_next_unit(rng));
        const float bend_amt = len * (0.02f + 0.22f * rng_next_unit(rng));
        const float bend_ang = (float)(nmath::PI_DOUBLE * 2.0) * rng_next_unit(rng);
        const Vec3 bend = (t0 * nmath_cos(bend_ang) + t1 * nmath_sin(bend_ang)) * bend_amt;
        const Vec3 tip = base + dir * len + bend;

        const Vec3 b0 = base + t0 * base_r;
        const Vec3 b1 = base + (t0 * -0.5f + t1 * 0.8660254f) * base_r;
        const Vec3 b2 = base + (t0 * -0.5f - t1 * 0.8660254f) * base_r;

        add_triangle_flat(obj, out, b0, b1, tip);
        add_triangle_flat(obj, out, b1, b2, tip);
        add_triangle_flat(obj, out, b2, b0, tip);
    }
}

void shell_spiral(object_t *obj, size_t resolution, float turns, float growth, float tube_radius)
{
    if (!obj) return;
    turns = std::max(0.5f, std::min(24.0f, turns));
    growth = std::max(0.01f, std::min(1.0f, growth));
    tube_radius = std::max(0.01f, std::min(1.0f, tube_radius));

    const size_t seg_u = std::max((size_t)96, (size_t)(resolution * turns * 2.0f));
    const size_t seg_v = std::max((size_t)12, resolution / 5);

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    std::vector<Vec3> centers(seg_u + 1);
    std::vector<Vec3> tangents(seg_u + 1);
    std::vector<Vec3> normals(seg_u + 1);
    std::vector<Vec3> binormals(seg_u + 1);
    std::vector<float> tube(seg_u + 1);

    for (size_t i = 0; i <= seg_u; ++i) {
        const float u = (float)i / (float)seg_u;
        const float a = (float)(nmath::PI_DOUBLE * 2.0) * turns * u;
        const float radial = 0.25f + growth * a * 0.18f;
        const float y = 0.08f * a;
        centers[i] = Vec3(radial * nmath_cos(a), y, radial * nmath_sin(a));
        tube[i] = tube_radius * (1.0f - 0.65f * u);
        if (tube[i] < tube_radius * 0.2f) tube[i] = tube_radius * 0.2f;
    }

    for (size_t i = 0; i <= seg_u; ++i) {
        const size_t ip = std::min(seg_u, i + 1);
        const size_t im = (i == 0) ? 0 : i - 1;
        Vec3 t = centers[ip] - centers[im];
        if (t.length() <= 1e-8f) t = Vec3(1, 0, 0);
        t.normalize();
        tangents[i] = t;
    }

    Vec3 prev_n(0, 1, 0);
    for (size_t i = 0; i <= seg_u; ++i) {
        Vec3 n = nmath::cross(prev_n, tangents[i]);
        if (n.length() <= 1e-8f) n = nmath::cross(Vec3(0, 1, 0), tangents[i]);
        if (n.length() <= 1e-8f) n = nmath::cross(Vec3(1, 0, 0), tangents[i]);
        if (n.length() <= 1e-8f) n = Vec3(0, 1, 0);
        n.normalize();
        Vec3 b = nmath::cross(tangents[i], n);
        if (b.length() <= 1e-8f) b = Vec3(0, 0, 1);
        b.normalize();
        n = nmath::cross(b, tangents[i]).normalized();
        normals[i] = n;
        binormals[i] = b;
        prev_n = n;
    }

    for (size_t i = 0; i <= seg_u; ++i) {
        const float u = (float)i / (float)seg_u;
        for (size_t j = 0; j <= seg_v; ++j) {
            const float v = (float)j / (float)seg_v;
            const float a = (float)(nmath::PI_DOUBLE * 2.0) * v;
            const float ca = nmath_cos(a);
            const float sa = nmath_sin(a);
            Vec3 dir = normals[i] * ca + binormals[i] * sa;
            if (dir.length() <= 1e-8f) dir = normals[i];
            dir.normalize();
            const Vec3 p = centers[i] + dir * tube[i];
            uv_t uv = {u, v};
            append_vertex(obj, p, dir, &uv);
        }
    }

    const int stride = (int)(seg_v + 1);
    for (size_t i = 0; i < seg_u; ++i) {
        for (size_t j = 0; j < seg_v; ++j) {
            const int i0 = (int)(i * (seg_v + 1) + j);
            const int i1 = i0 + 1;
            const int i2 = i0 + stride + 1;
            const int i3 = i0 + stride;
            append_quad(out, i0, i3, i2, i1, true);
        }
    }
}

void rock(object_t *obj, size_t resolution, int seed, float radius, float roughness, size_t octaves)
{
    if (!obj) return;
    if (radius <= 0.0f) radius = 1.0f;
    roughness = std::max(0.0f, std::min(2.0f, roughness));
    octaves = (size_t)clampi((int)octaves, 1, 8);

    const int iters = clampi((int)(resolution / 16), 1, 3);
    std::vector<Vec3> sphere_verts;
    std::vector<tri_t> sphere_faces;
    build_icosphere_data(sphere_verts, sphere_faces, iters);

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    std::vector<int> remap(sphere_verts.size(), -1);
    const uint32_t u_seed = (uint32_t)seed ^ 0x7f4a7c15u;
    for (size_t i = 0; i < sphere_verts.size(); ++i) {
        const Vec3 n = sphere_verts[i].normalized();
        const float nval = fbm_noise(n * 2.7f, u_seed, octaves);
        float rr = radius * (1.0f + roughness * 0.55f * nval);
        rr = std::max(radius * 0.18f, rr);
        const Vec3 p = n * rr;
        remap[i] = append_vertex(obj, p, p.normalized(), 0);
    }

    for (size_t i = 0; i < sphere_faces.size(); ++i) {
        const tri_t &t = sphere_faces[i];
        append_triangle(out, remap[t.a], remap[t.b], remap[t.c], false);
    }
}

void terrain(object_t *obj, size_t resolution, int seed, const nmath::Vector3f &dimensions, float noise_scale, size_t octaves, float lacunarity, float gain)
{
    if (!obj) return;

    const size_t seg = std::max((size_t)2, resolution);
    const float width = std::max(0.1f, (float)dimensions.x);
    const float height = std::max(0.0f, (float)dimensions.y);
    const float depth = std::max(0.1f, (float)dimensions.z);
    const float freq = std::max(0.01f, noise_scale);
    octaves = (size_t)clampi((int)octaves, 1, 10);
    lacunarity = std::max(1.01f, std::min(lacunarity, 8.0f));
    gain = std::max(0.05f, std::min(gain, 0.95f));

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    std::vector<float> heights((seg + 1) * (seg + 1), 0.0f);
    const uint32_t u_seed = (uint32_t)seed ^ 0x51b2d3a7u;

    for (size_t z = 0; z <= seg; ++z) {
        const float vz = (float)z / (float)seg;
        for (size_t x = 0; x <= seg; ++x) {
            const float vx = (float)x / (float)seg;
            const float px = (vx - 0.5f) * width;
            const float pz = (vz - 0.5f) * depth;
            const float nx = px * freq;
            const float nz = pz * freq;

            // Broad continent mass first, then mountain ridges on top.
            const float base = fbm_noise2_adv(nx * 0.35f, nz * 0.35f, u_seed, std::max((size_t)2, octaves), 1.9f, 0.55f);
            const float ridges = ridged_noise2_adv(nx, nz, u_seed ^ 0x9e3779b9u, std::max((size_t)2, octaves - 1), lacunarity, gain);

            // Shape lowlands and mountains into smoother natural relief.
            const float basin = base * 0.5f + 0.5f;
            const float continent = basin * basin;
            const float mountains = std::max(0.0f, base) * ridges;
            const float terrain_h = (continent * 0.65f) + (mountains * 0.7f) - 0.35f;

            heights[z * (seg + 1) + x] = terrain_h * height;
        }
    }

    for (size_t z = 0; z <= seg; ++z) {
        const float vz = (float)z / (float)seg;
        for (size_t x = 0; x <= seg; ++x) {
            const float vx = (float)x / (float)seg;
            const float px = (vx - 0.5f) * width;
            const float pz = (vz - 0.5f) * depth;
            const float py = heights[z * (seg + 1) + x];

            const size_t xl = (x > 0) ? x - 1 : x;
            const size_t xr = (x < seg) ? x + 1 : x;
            const size_t zd = (z > 0) ? z - 1 : z;
            const size_t zu = (z < seg) ? z + 1 : z;

            const float h_l = heights[z * (seg + 1) + xl];
            const float h_r = heights[z * (seg + 1) + xr];
            const float h_d = heights[zd * (seg + 1) + x];
            const float h_u = heights[zu * (seg + 1) + x];

            const float dx = (seg > 0) ? width / (float)seg : width;
            const float dz = (seg > 0) ? depth / (float)seg : depth;
            Vec3 normal(-(h_r - h_l) / std::max(1e-6f, (2.0f * dx)),
                         1.0f,
                        -(h_u - h_d) / std::max(1e-6f, (2.0f * dz)));
            if (normal.length() <= 1e-8f) normal = Vec3(0.0f, 1.0f, 0.0f);
            normal.normalize();

            uv_t uv = { vx, vz };
            append_vertex(obj, Vec3(px, py, pz), normal, &uv);
        }
    }

    for (size_t z = 0; z < seg; ++z) {
        for (size_t x = 0; x < seg; ++x) {
            const int i0 = (int)(x + z * (seg + 1));
            const int i1 = i0 + 1;
            const int i3 = i0 + (int)(seg + 1);
            const int i2 = i3 + 1;
            append_quad(out, i0, i3, i2, i1, true);
        }
    }
}

void draped_cloth_strip(object_t *obj, size_t resolution, const nmath::Vector3f &dimensions, float folds, float edge_lift, float curl, float taper, float sway, float asymmetry, float pinned)
{
    if (!obj) return;

    const size_t seg_u = std::max((size_t)8, resolution);
    const size_t seg_v = std::max((size_t)12, resolution + resolution / 2);
    const float width = std::max(0.1f, (float)dimensions.x);
    const float sag = std::max(0.0f, (float)dimensions.y);
    const float length = std::max(0.1f, (float)dimensions.z);
    folds = std::max(0.0f, std::min(folds, 12.0f));
    edge_lift = std::max(-1.0f, std::min(edge_lift, 1.0f));
    curl = std::max(-1.5f, std::min(curl, 1.5f));
    taper = std::max(-0.95f, std::min(taper, 0.95f));
    sway = std::max(-2.0f, std::min(sway, 2.0f));
    asymmetry = std::max(-1.0f, std::min(asymmetry, 1.0f));
    pinned = std::max(0.0f, std::min(pinned, 1.0f));

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    std::vector<Vec3> pos((seg_u + 1) * (seg_v + 1), Vec3(0.0f, 0.0f, 0.0f));

    for (size_t v = 0; v <= seg_v; ++v) {
        const float fv = (float)v / (float)seg_v;
        const float pin_curve = (nmath::scalar_t)1.0
                              - (float)std::pow((double)((nmath::scalar_t)1.0 - fv), (double)((nmath::scalar_t)(1.2f + pinned * 2.2f)));
        const float free_hang = nmath_sin(fv * nmath::PI);
        const float z = (fv - 0.5f) * length;
        const float center_drop = -sag * ((1.0f - pinned) * free_hang + pinned * pin_curve);
        const float tip_bias = -sag * (0.10f + 0.18f * pinned) * fv;
        const float width_scale = std::max(0.18f, 1.0f - taper * fv);
        const float center_sway = sway * width * 0.20f * nmath_sin(fv * nmath::PI * (1.2f + 0.4f * pinned));
        const float asym_bias = asymmetry * width * 0.08f * (0.3f + 0.7f * fv);

        for (size_t u = 0; u <= seg_u; ++u) {
            const float fu = (float)u / (float)seg_u;
            const float side = fu * 2.0f - 1.0f;
            const float edge = std::fabs(side);
            const float x_base = side * (width * 0.5f * width_scale);

            const float fold_env = free_hang;
            const float phase_skew = asymmetry * side * 0.8f + sway * 0.3f;
            const float fold_wave = nmath_sin((float)(folds * nmath::PI * 2.0f) * fv + side * 0.65f + phase_skew);
            const float fold_offset = fold_wave * width * 0.035f * fold_env * (0.85f + 0.15f * width_scale);

            const float edge_raise = edge_lift * sag * (edge * edge) * (0.35f + 0.65f * fold_env) * (1.0f + asymmetry * side * 0.35f);
            const float center_channel = -sag * (0.05f + 0.06f * pinned) * (1.0f - edge) * fold_env;
            const float side_bias = asymmetry * sag * 0.10f * side * (0.2f + 0.8f * fv);
            const float y = center_drop + tip_bias + edge_raise + center_channel + side_bias;

            const float twist = curl * fold_env * side * (0.45f + 0.55f * fv) + asymmetry * 0.18f * fv;
            const float x = x_base + fold_offset + center_sway + asym_bias;
            const float y_twist = y + nmath_sin(twist) * edge * width * 0.08f;
            const float z_twist = z + nmath_abs(side) * length * 0.05f * (1.0f - nmath_cos(twist))
                                    + sway * length * 0.035f * side * fold_env;

            pos[v * (seg_u + 1) + u] = Vec3(x, y_twist, z_twist);
        }
    }

    for (size_t v = 0; v <= seg_v; ++v) {
        const float fv = (float)v / (float)seg_v;
        for (size_t u = 0; u <= seg_u; ++u) {
            const float fu = (float)u / (float)seg_u;
            const size_t ul = (u > 0) ? u - 1 : u;
            const size_t ur = (u < seg_u) ? u + 1 : u;
            const size_t vd = (v > 0) ? v - 1 : v;
            const size_t vu = (v < seg_v) ? v + 1 : v;

            const Vec3 dpdu = pos[v * (seg_u + 1) + ur] - pos[v * (seg_u + 1) + ul];
            const Vec3 dpdv = pos[vu * (seg_u + 1) + u] - pos[vd * (seg_u + 1) + u];
            Vec3 normal = nmath::cross(dpdv, dpdu);
            if (normal.length() <= 1e-8f) normal = Vec3(0.0f, 1.0f, 0.0f);
            normal.normalize();

            uv_t uv = { fu, fv };
            append_vertex(obj, pos[v * (seg_u + 1) + u], normal, &uv);
        }
    }

    for (size_t v = 0; v < seg_v; ++v) {
        for (size_t u = 0; u < seg_u; ++u) {
            const int i0 = (int)(u + v * (seg_u + 1));
            const int i1 = i0 + 1;
            const int i3 = i0 + (int)(seg_u + 1);
            const int i2 = i3 + 1;
            append_quad(out, i0, i3, i2, i1, true);
        }
    }
}

void chain_link(object_t *obj, size_t resolution, size_t count, float major_radius, float minor_radius, float spacing, const std::vector<nmath::Vector3f> &spline)
{
    if (!obj) return;
    count = (size_t)clampi((int)count, 1, 128);
    major_radius = std::max(0.05f, major_radius);
    minor_radius = std::max(0.01f, std::min(minor_radius, major_radius * 0.95f));
    spacing = std::max(0.5f, std::min(2.0f, spacing));

    const size_t seg_u = std::max((size_t)24, resolution);
    const size_t seg_v = std::max((size_t)12, resolution / 2);
    // Use center pitch near major radius so orthogonal neighbors truly thread through.
    const float base_pitch = major_radius * 1.02f + minor_radius * 0.10f;
    const float pitch = std::max(1e-4f, base_pitch * spacing);

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    std::vector<Vec3> path_pts;
    if (spline.size() >= 2) {
        path_pts.reserve(spline.size());
        for (size_t i = 0; i < spline.size(); ++i) path_pts.push_back(Vec3(spline[i].x, spline[i].y, spline[i].z));
    }

    if (path_pts.size() >= 2) {
        std::vector<float> cum_lengths;
        build_polyline_lengths(path_pts, cum_lengths);
        const float total_len = cum_lengths.empty() ? 0.0f : cum_lengths.back();
        if (total_len > 1e-6f) {
            const float used_len = pitch * (float)(count > 0 ? (count - 1) : 0);
            const float start_s = std::max(0.0f, 0.5f * (total_len - used_len));
            Vec3 prev_n(0, 1, 0);

            for (size_t i = 0; i < count; ++i) {
                float s = start_s + pitch * (float)i;
                if (s < 0.0f) s = 0.0f;
                if (s > total_len) s = total_len;

                const Vec3 center = sample_polyline(path_pts, cum_lengths, s);
                const float eps = std::max(1e-3f, pitch * 0.2f);
                const Vec3 p0 = sample_polyline(path_pts, cum_lengths, std::max(0.0f, s - eps));
                const Vec3 p1 = sample_polyline(path_pts, cum_lengths, std::min(total_len, s + eps));
                const Vec3 tangent = safe_normalized(p1 - p0, Vec3(1, 0, 0));

                Vec3 n = prev_n - tangent * nmath::dot(prev_n, tangent);
                n = safe_normalized(n, nmath::cross(tangent, Vec3(0, 1, 0)));
                if (n.length() <= 1e-8f) n = safe_normalized(nmath::cross(tangent, Vec3(1, 0, 0)), Vec3(0, 1, 0));
                Vec3 b = safe_normalized(nmath::cross(tangent, n), Vec3(0, 0, 1));
                n = safe_normalized(nmath::cross(b, tangent), n);
                prev_n = n;

                const Vec3 axis = (i % 2 == 0) ? n : b;
                append_torus_link(obj, out, center, axis, major_radius, minor_radius, seg_u, seg_v);
            }
            return;
        }
    }

    const float center_offset = ((float)count - 1.0f) * 0.5f;
    for (size_t i = 0; i < count; ++i) {
        const float x = ((float)i - center_offset) * pitch;
        const Vec3 center(x, 0.0f, 0.0f);
        const Vec3 axis = (i % 2 == 0) ? Vec3(0, 1, 0) : Vec3(0, 0, 1);
        append_torus_link(obj, out, center, axis, major_radius, minor_radius, seg_u, seg_v);
    }
}

void lathe(object_t *obj, const std::vector<nmath::Vector2f> &profile, size_t resolution, bool cap_ends)
{
    if (!obj) return;
    const size_t seg_u = std::max((size_t)12, resolution);

    std::vector<Vec3> pts;
    if (profile.size() >= 2) {
        pts.reserve(profile.size());
        for (size_t i = 0; i < profile.size(); ++i) {
            const float r = std::max(0.0f, (float)profile[i].x);
            pts.push_back(Vec3(r, profile[i].y, 0.0f));
        }
    } else {
        // Fallback vase-like profile.
        pts.push_back(Vec3(0.00f, -1.00f, 0.0f));
        pts.push_back(Vec3(0.34f, -0.94f, 0.0f));
        pts.push_back(Vec3(0.52f, -0.55f, 0.0f));
        pts.push_back(Vec3(0.42f, -0.10f, 0.0f));
        pts.push_back(Vec3(0.58f,  0.35f, 0.0f));
        pts.push_back(Vec3(0.26f,  0.84f, 0.0f));
        pts.push_back(Vec3(0.18f,  1.00f, 0.0f));
    }

    if (pts.size() < 2) return;

    std::vector<Vec3> nrms(pts.size(), Vec3(1, 0, 0));
    for (size_t i = 0; i < pts.size(); ++i) {
        const size_t im = (i == 0) ? 0 : i - 1;
        const size_t ip = (i + 1 < pts.size()) ? i + 1 : pts.size() - 1;
        const Vec3 t = pts[ip] - pts[im];
        Vec3 n(t.y, -t.x, 0.0f);
        if (n.length() <= 1e-8f) n = Vec3(1, 0, 0);
        n.normalize();
        nrms[i] = n;
    }

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();
    build_revolution_band(obj, out, seg_u, pts.size() - 1, pts, nrms);

    if (!cap_ends) return;

    const float y0 = pts.front().y;
    const float r0 = std::max(0.0f, (float)pts.front().x);
    const float y1 = pts.back().y;
    const float r1 = std::max(0.0f, (float)pts.back().x);

    if (r0 > 1e-6f) {
        const int c0 = append_vertex(obj, Vec3(0, y0, 0), Vec3(0, -1, 0), 0);
        std::vector<int> ring;
        ring.reserve(seg_u + 1);
        for (size_t i = 0; i <= seg_u; ++i) {
            const float u = (float)i / (float)seg_u;
            const float a = (float)(nmath::PI_DOUBLE * 2.0) * u;
            ring.push_back(append_vertex(obj, Vec3(r0 * nmath_cos(a), y0, r0 * nmath_sin(a)), Vec3(0, -1, 0), 0));
        }
        for (size_t i = 0; i < seg_u; ++i) append_triangle(out, c0, ring[i + 1], ring[i], false);
    }

    if (r1 > 1e-6f) {
        const int c1 = append_vertex(obj, Vec3(0, y1, 0), Vec3(0, 1, 0), 0);
        std::vector<int> ring;
        ring.reserve(seg_u + 1);
        for (size_t i = 0; i <= seg_u; ++i) {
            const float u = (float)i / (float)seg_u;
            const float a = (float)(nmath::PI_DOUBLE * 2.0) * u;
            ring.push_back(append_vertex(obj, Vec3(r1 * nmath_cos(a), y1, r1 * nmath_sin(a)), Vec3(0, 1, 0), 0));
        }
        for (size_t i = 0; i < seg_u; ++i) append_triangle(out, c1, ring[i], ring[i + 1], false);
    }
}

void gear(object_t *obj, size_t resolution, size_t tooth_count,
          float tooth_depth, float inner_radius, float outer_radius, float height)
{
    if (!obj) return;
    tooth_count  = std::max((size_t)3,  std::min((size_t)128, tooth_count));
    outer_radius = std::max(0.1f, outer_radius);
    inner_radius = std::max(0.0f, std::min(outer_radius * 0.85f, inner_radius));
    tooth_depth  = std::max(0.01f, std::min((outer_radius - inner_radius) * 0.8f, tooth_depth));
    height       = std::max(0.01f, height);

    const float half_h = height * 0.5f;
    const float root_r = outer_radius - tooth_depth;
    const float tw     = 0.4f;  /* tooth plateau width fraction of pitch */

    const size_t pts = tooth_count * 4;
    std::vector<float> ang(pts), rad(pts);

    for (size_t t = 0; t < tooth_count; ++t) {
        const float base  = (float)(nmath::PI_DOUBLE * 2.0) * (float)t / (float)tooth_count;
        const float pitch = (float)(nmath::PI_DOUBLE * 2.0) / (float)tooth_count;
        ang[t*4+0] = base;                              rad[t*4+0] = root_r;
        ang[t*4+1] = base + pitch * (0.5f - tw * 0.5f); rad[t*4+1] = outer_radius;
        ang[t*4+2] = base + pitch *  0.5f;              rad[t*4+2] = outer_radius;
        ang[t*4+3] = base + pitch * (0.5f + tw * 0.5f); rad[t*4+3] = outer_radius;
    }

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    for (size_t i = 0; i < pts; ++i) {
        const size_t j = (i + 1) % pts;
        const Vec3 topi(rad[i]*nmath_cos(ang[i]),  half_h, rad[i]*nmath_sin(ang[i]));
        const Vec3 topj(rad[j]*nmath_cos(ang[j]),  half_h, rad[j]*nmath_sin(ang[j]));
        const Vec3 boti(rad[i]*nmath_cos(ang[i]), -half_h, rad[i]*nmath_sin(ang[i]));
        const Vec3 botj(rad[j]*nmath_cos(ang[j]), -half_h, rad[j]*nmath_sin(ang[j]));

        /* Top & bottom caps (+y / -y normals) */
        if (inner_radius <= 1e-4f) {
            add_triangle_flat(obj, out, Vec3(0, half_h, 0), topj, topi);
            add_triangle_flat(obj, out, Vec3(0,-half_h, 0), boti, botj);
        } else {
            const Vec3 itopi(inner_radius*nmath_cos(ang[i]),  half_h, inner_radius*nmath_sin(ang[i]));
            const Vec3 itopj(inner_radius*nmath_cos(ang[j]),  half_h, inner_radius*nmath_sin(ang[j]));
            const Vec3 iboti(inner_radius*nmath_cos(ang[i]), -half_h, inner_radius*nmath_sin(ang[i]));
            const Vec3 ibotj(inner_radius*nmath_cos(ang[j]), -half_h, inner_radius*nmath_sin(ang[j]));
            add_triangle_flat(obj, out, itopi, topj,  topi);
            add_triangle_flat(obj, out, itopi, itopj, topj);
            add_triangle_flat(obj, out, iboti, boti,  botj);
            add_triangle_flat(obj, out, iboti, botj,  ibotj);
        }

        /* Outer side face */
        add_triangle_flat(obj, out, boti, topi, topj);
        add_triangle_flat(obj, out, boti, topj, botj);
    }

    /* Inner bore */
    if (inner_radius > 1e-4f) {
        const size_t bore_seg = std::max((size_t)24, resolution);
        for (size_t i = 0; i < bore_seg; ++i) {
            const float a0 = (float)(nmath::PI_DOUBLE*2.0) * (float)i       / (float)bore_seg;
            const float a1 = (float)(nmath::PI_DOUBLE*2.0) * (float)(i + 1) / (float)bore_seg;
            const Vec3 b0(inner_radius*nmath_cos(a0), -half_h, inner_radius*nmath_sin(a0));
            const Vec3 b1(inner_radius*nmath_cos(a1), -half_h, inner_radius*nmath_sin(a1));
            const Vec3 t0(inner_radius*nmath_cos(a0),  half_h, inner_radius*nmath_sin(a0));
            const Vec3 t1(inner_radius*nmath_cos(a1),  half_h, inner_radius*nmath_sin(a1));
            add_triangle_flat(obj, out, b0, b1, t0);
            add_triangle_flat(obj, out, b1, t1, t0);
        }
    }
}

void spring(object_t *obj, size_t resolution, float coils, float wire_radius,
            float spring_radius, float height)
{
    if (!obj) return;
    coils         = std::max(0.5f,  std::min(50.0f, coils));
    wire_radius   = std::max(0.01f, wire_radius);
    spring_radius = std::max(wire_radius * 2.0f, spring_radius);
    height        = std::max(wire_radius * 2.0f, height);

    const size_t seg_u = std::max((size_t)64, (size_t)(resolution * coils * 0.5f));
    const size_t seg_v = std::max((size_t)8,  resolution / 8);

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    std::vector<Vec3> centers(seg_u + 1);
    std::vector<Vec3> normals_fr(seg_u + 1), binormals_fr(seg_u + 1);

    for (size_t i = 0; i <= seg_u; ++i) {
        const float t   = (float)i / (float)seg_u;
        const float ang = (float)(nmath::PI_DOUBLE) * coils * t;
        centers[i] = Vec3(spring_radius * nmath_cos(ang),
                          height * (t - 0.5f),
                          spring_radius * nmath_sin(ang));
    }

    Vec3 prev_n(0, 0, 1);
    for (size_t i = 0; i <= seg_u; ++i) {
        const size_t ip = std::min(seg_u, i + 1);
        const size_t im = i > 0 ? i - 1 : 0;
        Vec3 t = centers[ip] - centers[im];
        if (t.length() <= 1e-8f) t = Vec3(0, 1, 0);
        t.normalize();

        Vec3 n = nmath::cross(prev_n, t);
        if (n.length() <= 1e-8f) n = nmath::cross(Vec3(0, 0, 1), t);
        if (n.length() <= 1e-8f) n = nmath::cross(Vec3(1, 0, 0), t);
        if (n.length() <= 1e-8f) n = Vec3(0, 1, 0);
        n.normalize();
        Vec3 b = nmath::cross(t, n);
        if (b.length() <= 1e-8f) b = Vec3(0, 0, 1);
        b.normalize();
        n = nmath::cross(b, t).normalized();
        normals_fr[i]   = n;
        binormals_fr[i] = b;
        prev_n = n;
    }

    for (size_t i = 0; i <= seg_u; ++i) {
        const float u = (float)i / (float)seg_u;
        for (size_t j = 0; j <= seg_v; ++j) {
            const float v   = (float)j / (float)seg_v;
            const float ang = (float)(nmath::PI_DOUBLE * 2.0) * v;
            Vec3 dir = normals_fr[i] * nmath_cos(ang) + binormals_fr[i] * nmath_sin(ang);
            if (dir.length() <= 1e-8f) dir = normals_fr[i];
            dir.normalize();
            uv_t uv = {u, v};
            append_vertex(obj, centers[i] + dir * wire_radius, dir, &uv);
        }
    }

    const int stride = (int)(seg_v + 1);
    for (size_t i = 0; i < seg_u; ++i) {
        for (size_t j = 0; j < seg_v; ++j) {
            const int i0 = (int)(i * (seg_v + 1) + j);
            const int i1 = i0 + 1;
            const int i2 = i0 + stride + 1;
            const int i3 = i0 + stride;
            append_quad(out, i0, i3, i2, i1, true);
        }
    }
}

void hemisphere(object_t *obj, size_t resolution)
{
    if (!obj) return;
    const size_t seg_u = std::max((size_t)8,  resolution / 4);
    const size_t seg_v = std::max((size_t)16, resolution);

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    /* Dome: phi sweeps from 0 (top pole) to π/2 (equator).
       PI_DOUBLE = 2π, so multiply by 0.25 to get π/2. */
    for (size_t iu = 0; iu <= seg_u; ++iu) {
        const float phi = (float)(nmath::PI_DOUBLE * 0.25) * (float)iu / (float)seg_u;
        const float sp = nmath_sin(phi);
        const float cp = nmath_cos(phi);
        for (size_t iv = 0; iv <= seg_v; ++iv) {
            const float theta = (float)(nmath::PI_DOUBLE * 2.0) * (float)iv / (float)seg_v;
            const Vec3 n(sp * nmath_cos(theta), cp, sp * nmath_sin(theta));
            uv_t uv = {(float)iv / (float)seg_v, (float)iu / (float)seg_u};
            append_vertex(obj, n, n, &uv);
        }
    }

    const int stride = (int)(seg_v + 1);
    for (size_t iu = 0; iu < seg_u; ++iu) {
        for (size_t iv = 0; iv < seg_v; ++iv) {
            const int i0 = (int)(iu       * (seg_v + 1) + iv);
            const int i1 = i0 + 1;
            const int i2 = i0 + stride + 1;
            const int i3 = i0 + stride;
            append_quad(out, i0, i3, i2, i1, true);
        }
    }

    /* Bottom disc cap at y = 0, normal pointing down */
    const int c = append_vertex(obj, Vec3(0, 0, 0), Vec3(0, -1, 0));
    for (size_t iv = 0; iv < seg_v; ++iv) {
        const float a0 = (float)(nmath::PI_DOUBLE * 2.0) * (float)(iv)     / (float)seg_v;
        const float a1 = (float)(nmath::PI_DOUBLE * 2.0) * (float)(iv + 1) / (float)seg_v;
        const int r0 = append_vertex(obj, Vec3(nmath_cos(a0), 0.0f, nmath_sin(a0)), Vec3(0,-1,0));
        const int r1 = append_vertex(obj, Vec3(nmath_cos(a1), 0.0f, nmath_sin(a1)), Vec3(0,-1,0));
        append_triangle(out, c, r1, r0, false);
    }
}

void disc(object_t *obj, size_t resolution, float inner_radius, float outer_radius)
{
    if (!obj) return;
    outer_radius = std::max(0.01f, outer_radius);
    inner_radius = std::max(0.0f,  std::min(outer_radius * 0.95f, inner_radius));
    const size_t seg = std::max((size_t)16, resolution);

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    if (inner_radius <= 1e-4f) {
        /* Solid disc */
        const int center = append_vertex(obj, Vec3(0, 0, 0), Vec3(0, 1, 0));
        for (size_t i = 0; i < seg; ++i) {
            const float a0 = (float)(nmath::PI_DOUBLE * 2.0) * (float)i       / (float)seg;
            const float a1 = (float)(nmath::PI_DOUBLE * 2.0) * (float)(i + 1) / (float)seg;
            const int r0 = append_vertex(obj, Vec3(outer_radius*nmath_cos(a0), 0, outer_radius*nmath_sin(a0)), Vec3(0,1,0));
            const int r1 = append_vertex(obj, Vec3(outer_radius*nmath_cos(a1), 0, outer_radius*nmath_sin(a1)), Vec3(0,1,0));
            append_triangle(out, center, r0, r1, false);
        }
    } else {
        /* Annular disc */
        for (size_t i = 0; i < seg; ++i) {
            const float a0 = (float)(nmath::PI_DOUBLE * 2.0) * (float)i       / (float)seg;
            const float a1 = (float)(nmath::PI_DOUBLE * 2.0) * (float)(i + 1) / (float)seg;
            const Vec3 o0(outer_radius*nmath_cos(a0), 0, outer_radius*nmath_sin(a0));
            const Vec3 o1(outer_radius*nmath_cos(a1), 0, outer_radius*nmath_sin(a1));
            const Vec3 i0v(inner_radius*nmath_cos(a0), 0, inner_radius*nmath_sin(a0));
            const Vec3 i1v(inner_radius*nmath_cos(a1), 0, inner_radius*nmath_sin(a1));
            add_triangle_flat(obj, out, i0v, o1, o0);
            add_triangle_flat(obj, out, i0v, i1v, o1);
        }
    }
}

void star(object_t *obj, size_t resolution, size_t points, float inner_radius,
          float outer_radius, float height)
{
    if (!obj) return;
    (void)resolution;
    points       = std::max((size_t)3,  std::min((size_t)32, points));
    outer_radius = std::max(0.1f, outer_radius);
    inner_radius = std::max(0.01f, std::min(outer_radius * 0.95f, inner_radius));
    height       = std::max(0.01f, height);

    const float half_h = height * 0.5f;
    const size_t pts   = points * 2;  /* alternating tip / valley */
    std::vector<float> ang(pts), rad(pts);

    for (size_t p = 0; p < points; ++p) {
        const float base  = (float)(nmath::PI_DOUBLE * 2.0) * (float)p / (float)points;
        const float pitch = (float)(nmath::PI_DOUBLE * 2.0) / (float)points;
        ang[p*2 + 0] = base;
        rad[p*2 + 0] = outer_radius;
        ang[p*2 + 1] = base + pitch * 0.5f;
        rad[p*2 + 1] = inner_radius;
    }

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    for (size_t i = 0; i < pts; ++i) {
        const size_t j = (i + 1) % pts;
        const Vec3 topi(rad[i]*nmath_cos(ang[i]),  half_h, rad[i]*nmath_sin(ang[i]));
        const Vec3 topj(rad[j]*nmath_cos(ang[j]),  half_h, rad[j]*nmath_sin(ang[j]));
        const Vec3 boti(rad[i]*nmath_cos(ang[i]), -half_h, rad[i]*nmath_sin(ang[i]));
        const Vec3 botj(rad[j]*nmath_cos(ang[j]), -half_h, rad[j]*nmath_sin(ang[j]));

        add_triangle_flat(obj, out, Vec3(0, half_h, 0),  topj, topi);
        add_triangle_flat(obj, out, Vec3(0,-half_h, 0),  boti, botj);
        add_triangle_flat(obj, out, boti, topi, topj);
        add_triangle_flat(obj, out, boti, topj, botj);
    }
}

void superellipsoid(object_t *obj, size_t resolution, float e1, float e2)
{
    if (!obj) return;
    e1 = std::max(0.1f, std::min(4.0f, e1));
    e2 = std::max(0.1f, std::min(4.0f, e2));

    const size_t seg_u = std::max((size_t)16, resolution / 2);
    const size_t seg_v = std::max((size_t)16, resolution);
    const size_t stride = seg_v + 1;

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    /* Compute all positions first for finite-difference normals */
    std::vector<Vec3> pos((seg_u + 1) * stride);
    for (size_t iu = 0; iu <= seg_u; ++iu) {
        const float t = (float)iu / (float)seg_u;
        /* PI_DOUBLE = 2π; multiply by 0.25 to get π/2, then shift → [-π/2, π/2] */
        const float u = (float)(nmath::PI_DOUBLE * 0.5) * (t - 0.5f);
        const float cu = nmath_cos(u), su = nmath_sin(u);
        const float acu = std::max(1e-9f, std::abs(cu));
        const float asu = std::max(1e-9f, std::abs(su));
        const float cu_e1 = (cu >= 0.0f ? 1.0f : -1.0f) * std::pow(acu, e1);
        const float su_e1 = (su >= 0.0f ? 1.0f : -1.0f) * std::pow(asu, e1);

        for (size_t iv = 0; iv <= seg_v; ++iv) {
            const float s = (float)iv / (float)seg_v;
            /* PI_DOUBLE = 2π; this gives [-π, π] for the longitude */
            const float v = (float)(nmath::PI_DOUBLE) * (s - 0.5f);
            const float cv = nmath_cos(v), sv = nmath_sin(v);
            const float acv = std::max(1e-9f, std::abs(cv));
            const float asv = std::max(1e-9f, std::abs(sv));
            const float cv_e2 = (cv >= 0.0f ? 1.0f : -1.0f) * std::pow(acv, e2);
            const float sv_e2 = (sv >= 0.0f ? 1.0f : -1.0f) * std::pow(asv, e2);
            pos[iu * stride + iv] = Vec3(cu_e1 * cv_e2, su_e1, cu_e1 * sv_e2);
        }
    }

    /* Add vertices with finite-difference normals */
    std::vector<int> idx((seg_u + 1) * stride);
    for (size_t iu = 0; iu <= seg_u; ++iu) {
        for (size_t iv = 0; iv <= seg_v; ++iv) {
            const size_t iu0 = iu > 0      ? iu - 1 : iu;
            const size_t iu1 = iu < seg_u  ? iu + 1 : iu;
            const size_t iv0 = iv > 0      ? iv - 1 : iv;
            const size_t iv1 = iv < seg_v  ? iv + 1 : iv;
            Vec3 du = pos[iu1 * stride + iv] - pos[iu0 * stride + iv];
            Vec3 dv = pos[iu  * stride + iv1] - pos[iu  * stride + iv0];
            Vec3 n  = nmath::cross(du, dv);
            if (n.length() <= 1e-8f) n = pos[iu * stride + iv];
            if (n.length() <= 1e-8f) n = Vec3(0, 1, 0);
            n.normalize();
            uv_t uv = {(float)iv / (float)seg_v, (float)iu / (float)seg_u};
            idx[iu * stride + iv] = append_vertex(obj, pos[iu * stride + iv], n, &uv);
        }
    }

    for (size_t iu = 0; iu < seg_u; ++iu) {
        for (size_t iv = 0; iv < seg_v; ++iv) {
            const int i0 = idx[ iu      * stride + iv    ];
            const int i1 = idx[ iu      * stride + iv + 1];
            const int i2 = idx[(iu + 1) * stride + iv + 1];
            const int i3 = idx[(iu + 1) * stride + iv    ];
            append_quad(out, i0, i3, i2, i1, true);
        }
    }
}

void crystal(object_t *obj, size_t resolution, size_t count, float radius,
             float height, float tip_height, int seed)
{
    if (!obj) return;
    count      = std::max((size_t)1, std::min((size_t)32, count));
    radius     = std::max(0.05f, radius);
    height     = std::max(0.1f,  height);
    tip_height = std::max(0.05f, tip_height);
    const size_t facets = (size_t)clampi((int)(resolution / 8) + 4, 5, 16);

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    rng_t rng;
    rng.state = (uint32_t)seed ^ 0xb3a1c5e7u;
    if (rng.state == 0u) rng.state = 1u;

    for (size_t c = 0; c < count; ++c) {
        /* Random position within cluster radius */
        const float a_pos = (float)(nmath::PI_DOUBLE * 2.0) * rng_next_unit(rng);
        const float r_pos = radius * 0.8f * std::sqrt(rng_next_unit(rng));
        const float cx = r_pos * nmath_cos(a_pos);
        const float cz = r_pos * nmath_sin(a_pos);

        /* Random orientation and size */
        const float rot     = (float)(nmath::PI_DOUBLE * 2.0) * rng_next_unit(rng);
        const float h_scale = 0.4f + 0.6f * rng_next_unit(rng);
        const float prism_h = height * h_scale;
        const float tip_h   = tip_height * (0.6f + 0.8f * rng_next_unit(rng));
        const float prism_r = radius * 0.22f * (0.5f + 0.8f * rng_next_unit(rng));

        /* Slight random tilt */
        const float tilt     = 0.25f * rng_next_unit(rng);
        const float tilt_ang = (float)(nmath::PI_DOUBLE * 2.0) * rng_next_unit(rng);
        const Vec3 tilt_dir(nmath_cos(tilt_ang) * nmath_sin(tilt),
                            nmath_cos(tilt),
                            nmath_sin(tilt_ang) * nmath_sin(tilt));

        std::vector<Vec3> base_ring(facets), top_ring(facets);
        for (size_t f = 0; f < facets; ++f) {
            const float a = (float)(nmath::PI_DOUBLE * 2.0) * (float)f / (float)facets + rot;
            const Vec3 off(prism_r * nmath_cos(a), 0, prism_r * nmath_sin(a));
            base_ring[f] = Vec3(cx, 0.0f, cz) + off;
            top_ring[f]  = Vec3(cx, prism_h, cz) + off;
        }
        const Vec3 apex(cx + tilt_dir.x * tip_h,
                        prism_h + tilt_dir.y * tip_h,
                        cz + tilt_dir.z * tip_h);

        /* Bottom cap (normal -y) */
        for (size_t f = 0; f < facets; ++f) {
            const size_t g = (f + 1) % facets;
            add_triangle_flat(obj, out, Vec3(cx, 0.0f, cz), base_ring[g], base_ring[f]);
        }
        /* Prism side faces */
        for (size_t f = 0; f < facets; ++f) {
            const size_t g = (f + 1) % facets;
            add_triangle_flat(obj, out, base_ring[g], base_ring[f], top_ring[f]);
            add_triangle_flat(obj, out, base_ring[g], top_ring[f],  top_ring[g]);
        }
        /* Pyramid tip faces */
        for (size_t f = 0; f < facets; ++f) {
            const size_t g = (f + 1) % facets;
            add_triangle_flat(obj, out, top_ring[f], apex, top_ring[g]);
        }
    }
}

/* ---- tree recursive helper ---- */
struct tree_params_t {
    int   branch_count;
    float branch_angle;
    float len_scale;
    float rad_scale;
    rng_t *rng;
};

static void gen_tree_branch(object_t *obj, shape_t &out,
    const Vec3 &base, const Vec3 &dir,
    float length, float radius, int depth,
    tree_params_t &p)
{
    if (depth <= 0 || length < 0.01f || radius < 0.001f) return;

    const Vec3 tip = base + dir * length;
    const size_t seg = 6;
    const float tip_r = radius * 0.65f;

    Vec3 perp = nmath::cross(dir, Vec3(0, 1, 0));
    if (perp.length() <= 1e-6f) perp = nmath::cross(dir, Vec3(1, 0, 0));
    if (perp.length() <= 1e-6f) perp = Vec3(1, 0, 0);
    perp.normalize();
    const Vec3 binom = nmath::cross(dir, perp).normalized();

    std::vector<Vec3> base_verts(seg), tip_verts(seg);
    for (size_t i = 0; i < seg; ++i) {
        const float a = (float)(nmath::PI_DOUBLE * 2.0) * (float)i / (float)seg;
        const Vec3 d = perp * nmath_cos(a) + binom * nmath_sin(a);
        base_verts[i] = base + d * radius;
        tip_verts[i]  = tip  + d * tip_r;
    }

    for (size_t i = 0; i < seg; ++i) {
        const size_t j = (i + 1) % seg;
        add_triangle_flat(obj, out, base_verts[j], base_verts[i], tip_verts[i]);
        add_triangle_flat(obj, out, base_verts[j], tip_verts[i],  tip_verts[j]);
    }
    /* Tip cap */
    for (size_t i = 0; i < seg; ++i) {
        const size_t j = (i + 1) % seg;
        add_triangle_flat(obj, out, tip, tip_verts[j], tip_verts[i]);
    }

    for (int b = 0; b < p.branch_count; ++b) {
        const float angle = p.branch_angle * (0.6f + 0.8f * rng_next_unit(*p.rng));
        const float twist = (float)(nmath::PI_DOUBLE * 2.0) * (float)b / (float)p.branch_count
                           + (rng_next_unit(*p.rng) - 0.5f) * 1.2f;
        const Vec3 side = (perp * nmath_cos(twist) + binom * nmath_sin(twist)).normalized();
        const Vec3 child_dir = (dir * nmath_cos(angle) + side * nmath_sin(angle)).normalized();
        const float frac = 0.5f + 0.35f * rng_next_unit(*p.rng);
        gen_tree_branch(obj, out, base + dir * (length * frac), child_dir,
                        length * p.len_scale, radius * p.rad_scale, depth - 1, p);
    }
}

void tree(object_t *obj, size_t /*resolution*/, int depth, int branch_count,
          float branch_angle, float trunk_height, float trunk_radius, int seed)
{
    if (!obj) return;
    depth        = clampi(depth, 1, 7);
    branch_count = clampi(branch_count, 1, 6);
    branch_angle = std::max(0.1f, std::min((float)(nmath::PI_DOUBLE * 0.5), branch_angle));
    trunk_height = std::max(0.1f, trunk_height);
    trunk_radius = std::max(0.01f, trunk_radius);

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    rng_t rng;
    rng.state = (uint32_t)seed ^ 0x3c5fa1b7u;
    if (rng.state == 0u) rng.state = 1u;

    tree_params_t params;
    params.branch_count = branch_count;
    params.branch_angle = branch_angle;
    params.len_scale    = 0.68f;
    params.rad_scale    = 0.60f;
    params.rng          = &rng;

    gen_tree_branch(obj, out,
        Vec3(0, -trunk_height * 0.5f, 0),
        Vec3(0, 1, 0),
        trunk_height, trunk_radius, depth, params);
}

/* ---- coral recursive helper ---- */
struct coral_params_t {
    int   branch_count;
    float branch_angle;
    float len_scale;
    float rad_scale;
    rng_t *rng;
};

static void gen_coral_branch(object_t *obj, shape_t &out,
    const Vec3 &base, const Vec3 &dir,
    float length, float radius, int depth,
    coral_params_t &p)
{
    if (depth <= 0 || length < 0.008f || radius < 0.0008f) return;

    const Vec3 tip = base + dir * length;
    const size_t seg = 5;
    const float tip_r = radius * 0.55f;

    Vec3 perp = nmath::cross(dir, Vec3(0, 1, 0));
    if (perp.length() <= 1e-6f) perp = nmath::cross(dir, Vec3(1, 0, 0));
    if (perp.length() <= 1e-6f) perp = Vec3(1, 0, 0);
    perp.normalize();
    const Vec3 binom = nmath::cross(dir, perp).normalized();

    std::vector<Vec3> base_verts(seg), tip_verts(seg);
    for (size_t i = 0; i < seg; ++i) {
        const float a = (float)(nmath::PI_DOUBLE * 2.0) * (float)i / (float)seg;
        const Vec3 d = perp * nmath_cos(a) + binom * nmath_sin(a);
        base_verts[i] = base + d * radius;
        tip_verts[i]  = tip  + d * tip_r;
    }

    for (size_t i = 0; i < seg; ++i) {
        const size_t j = (i + 1) % seg;
        add_triangle_flat(obj, out, base_verts[j], base_verts[i], tip_verts[i]);
        add_triangle_flat(obj, out, base_verts[j], tip_verts[i],  tip_verts[j]);
    }
    /* Tip cap */
    for (size_t i = 0; i < seg; ++i) {
        const size_t j = (i + 1) % seg;
        add_triangle_flat(obj, out, tip, tip_verts[j], tip_verts[i]);
    }

    /* Coral branches spread radially into the upper hemisphere regardless of
       parent direction — this creates the bushy, spherical coral silhouette. */
    for (int b = 0; b < p.branch_count; ++b) {
        /* Evenly divide azimuth around full circle, plus small random jitter */
        const float azimuth = (float)(nmath::PI_DOUBLE * 2.0) * (float)b / (float)p.branch_count
                             + (rng_next_unit(*p.rng) - 0.5f) * 1.8f;
        /* Elevation in [20°, 70°] range — upward but spreading wide */
        const float elev_min = (float)(nmath::PI_DOUBLE * 0.5) * 0.22f;
        const float elev_max = (float)(nmath::PI_DOUBLE * 0.5) * 0.78f;
        const float elev = elev_min + rng_next_unit(*p.rng) * (elev_max - elev_min)
                          + p.branch_angle * (rng_next_unit(*p.rng) - 0.5f) * 0.4f;
        /* Build child direction in world space from spherical coords — ignores parent dir */
        Vec3 child_dir(nmath_cos(elev) * nmath_cos(azimuth),
                       nmath_sin(elev),
                       nmath_cos(elev) * nmath_sin(azimuth));
        child_dir.normalize();
        const float frac = 0.4f + 0.45f * rng_next_unit(*p.rng);
        gen_coral_branch(obj, out, base + dir * (length * frac), child_dir,
                         length * p.len_scale, radius * p.rad_scale, depth - 1, p);
    }
}

void coral(object_t *obj, size_t /*resolution*/, int depth, int branch_count,
           float branch_angle, float height, float branch_radius, int seed)
{
    if (!obj) return;
    depth         = clampi(depth, 1, 7);
    branch_count  = clampi(branch_count, 1, 8);
    branch_angle  = std::max(0.1f, std::min((float)(nmath::PI_DOUBLE * 0.6), branch_angle));
    height        = std::max(0.05f, height);
    branch_radius = std::max(0.005f, branch_radius);

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    rng_t rng;
    rng.state = (uint32_t)seed ^ 0x7a3d2b91u;
    if (rng.state == 0u) rng.state = 1u;

    coral_params_t params;
    params.branch_count = branch_count;
    params.branch_angle = branch_angle;
    params.len_scale    = 0.62f;
    params.rad_scale    = 0.55f;
    params.rng          = &rng;

    gen_coral_branch(obj, out,
        Vec3(0, -height * 0.5f, 0),
        Vec3(0, 1, 0),
        height, branch_radius, depth, params);
}

void displaced_sphere(object_t *obj, size_t resolution, float radius)
{
    if (!obj) return;
    if (radius <= 0.0f) radius = 1.0f;

    const int iters = (int)(resolution);
    std::vector<Vec3> verts;
    std::vector<tri_t> faces;
    build_icosphere_data(verts, faces, iters);

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    const float inv_pi  = 1.0f / (float)nmath::PI;
    const float inv_2pi = 1.0f / (2.0f * (float)nmath::PI);

    // Compute spherical (longitude/latitude) UV for every icosphere vertex.
    std::vector<uv_t> base_uv(verts.size());
    for (size_t i = 0; i < verts.size(); ++i) {
        const Vec3 &n = verts[i];
        base_uv[i].u = (float)nmath_atan2(n.z, n.x) * inv_2pi + 0.5f;
        base_uv[i].v = (float)nmath_asin(std::max(-1.0f, std::min(1.0f, (float)n.y))) * inv_pi + 0.5f;
    }

    std::vector<int> remap(verts.size(), -1);
    for (size_t i = 0; i < verts.size(); ++i)
        remap[i] = append_vertex(obj, verts[i] * radius, verts[i], &base_uv[i]);

    // Emit triangles; duplicate seam vertices so the UV atlas is contiguous.
    // A face straddles the seam when its u-range exceeds 0.5 — push the
    // low-u vertices past 1.0 so bilinear filtering wraps correctly.
    for (size_t i = 0; i < faces.size(); ++i) {
        const tri_t &t = faces[i];
        int ia = remap[t.a], ib = remap[t.b], ic = remap[t.c];
        const float ua = base_uv[t.a].u, ub = base_uv[t.b].u, uc = base_uv[t.c].u;
        const float umin = std::min(ua, std::min(ub, uc));
        const float umax = std::max(ua, std::max(ub, uc));

        if (umax - umin > 0.5f) {
            if (ua < 0.5f) {
                uv_t fx = {ua + 1.0f, base_uv[t.a].v};
                ia = append_vertex(obj, verts[t.a] * radius, verts[t.a], &fx);
            }
            if (ub < 0.5f) {
                uv_t fx = {ub + 1.0f, base_uv[t.b].v};
                ib = append_vertex(obj, verts[t.b] * radius, verts[t.b], &fx);
            }
            if (uc < 0.5f) {
                uv_t fx = {uc + 1.0f, base_uv[t.c].v};
                ic = append_vertex(obj, verts[t.c] * radius, verts[t.c], &fx);
            }
        }

        append_triangle(out, ia, ib, ic, true);
    }
}

    } /* namespace generator */
} /* namespace nmesh */
