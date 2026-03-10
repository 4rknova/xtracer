#include <algorithm>
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

static int clampi(int v, int lo, int hi)
{
    return std::max(lo, std::min(v, hi));
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

    iterations = clampi(iterations, 0, 3);
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
        add_triangle_flat(obj, shape, p100, p101, p111);
        add_triangle_flat(obj, shape, p100, p111, p110);
    }
    if (occupied.find(key(gx - 1, gy, gz)) == occupied.end()) {
        add_triangle_flat(obj, shape, p001, p000, p010);
        add_triangle_flat(obj, shape, p001, p010, p011);
    }
    if (occupied.find(key(gx, gy + 1, gz)) == occupied.end()) {
        add_triangle_flat(obj, shape, p010, p110, p111);
        add_triangle_flat(obj, shape, p010, p111, p011);
    }
    if (occupied.find(key(gx, gy - 1, gz)) == occupied.end()) {
        add_triangle_flat(obj, shape, p000, p001, p101);
        add_triangle_flat(obj, shape, p000, p101, p100);
    }
    if (occupied.find(key(gx, gy, gz + 1)) == occupied.end()) {
        add_triangle_flat(obj, shape, p101, p001, p011);
        add_triangle_flat(obj, shape, p101, p011, p111);
    }
    if (occupied.find(key(gx, gy, gz - 1)) == occupied.end()) {
        add_triangle_flat(obj, shape, p000, p100, p110);
        add_triangle_flat(obj, shape, p000, p110, p010);
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

void sierpinski_tetrahedron(object_t *obj, size_t resolution)
{
    if (!obj) return;
    const int depth = clampi((int)resolution, 1, 4);

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    const float s = 1.0f / nmath_sqrt(3.0f);
    std::vector<std::vector<Vec3> > tets;
    tets.push_back(std::vector<Vec3>{
        Vec3( s,  s,  s),
        Vec3(-s, -s,  s),
        Vec3(-s,  s, -s),
        Vec3( s, -s, -s)
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

    for (size_t i = 0; i < tets.size(); ++i) {
        const std::vector<Vec3> &t = tets[i];
        add_triangle_flat(obj, out, t[0], t[1], t[2]);
        add_triangle_flat(obj, out, t[0], t[3], t[1]);
        add_triangle_flat(obj, out, t[0], t[2], t[3]);
        add_triangle_flat(obj, out, t[1], t[3], t[2]);
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

    const float norm = 1.5f;
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

void mobius_strip(object_t *obj, size_t resolution)
{
    if (!obj) return;
    const size_t seg_u = std::max((size_t)64, resolution);
    const size_t seg_v = std::max((size_t)12, resolution / 4);
    const float R = 1.0f;
    const float W = 0.32f;

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

    } /* namespace generator */
} /* namespace nmesh */
