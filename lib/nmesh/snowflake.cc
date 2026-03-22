#include <algorithm>
#include <cmath>
#include <vector>

#include <nmath/precision.h>
#include <nmath/vector.h>

#include "snowflake.h"

namespace nmesh {
    namespace generator {

namespace {

typedef nmath::Vector3f Vec3;

typedef struct {
    float x;
    float z;
} pt2_t;

static pt2_t make_pt2(float x, float z)
{
    pt2_t p;
    p.x = x;
    p.z = z;
    return p;
}

static float area2(const std::vector<pt2_t> &poly)
{
    float a = 0.0f;
    const size_t n = poly.size();
    for (size_t i = 0; i < n; ++i) {
        const pt2_t &p = poly[i];
        const pt2_t &q = poly[(i + 1) % n];
        a += (p.x * q.z - q.x * p.z);
    }
    return a * 0.5f;
}

static float cross2(const pt2_t &a, const pt2_t &b, const pt2_t &c)
{
    return (b.x - a.x) * (c.z - a.z) - (b.z - a.z) * (c.x - a.x);
}

static bool point_in_tri(const pt2_t &p, const pt2_t &a, const pt2_t &b, const pt2_t &c)
{
    const float c0 = cross2(a, b, p);
    const float c1 = cross2(b, c, p);
    const float c2 = cross2(c, a, p);
    const bool has_neg = (c0 < 0.0f) || (c1 < 0.0f) || (c2 < 0.0f);
    const bool has_pos = (c0 > 0.0f) || (c1 > 0.0f) || (c2 > 0.0f);
    return !(has_neg && has_pos);
}

static int append_vertex(object_t *obj, const Vec3 &p)
{
    const int idx = (int)(obj->attributes.v.size() / 3);
    obj->attributes.v.push_back(p.x);
    obj->attributes.v.push_back(p.y);
    obj->attributes.v.push_back(p.z);
    obj->attributes.n.push_back(0.0f);
    obj->attributes.n.push_back(1.0f);
    obj->attributes.n.push_back(0.0f);
    obj->attributes.uv.push_back(p.x + 0.5f);
    obj->attributes.uv.push_back(p.z + 0.5f);
    return idx;
}

static void append_triangle(shape_t &shape, int a, int b, int c)
{
    index_t ia;
    ia.v = a; ia.n = a; ia.uv = a;
    index_t ib;
    ib.v = b; ib.n = b; ib.uv = b;
    index_t ic;
    ic.v = c; ic.n = c; ic.uv = c;

    shape.mesh.indices.push_back(ia);
    shape.mesh.indices.push_back(ib);
    shape.mesh.indices.push_back(ic);
}

} /* namespace */

void snowflake(object_t *obj, size_t iterations)
{
    if (!obj) return;
    if (iterations > 6) iterations = 6;

    std::vector<pt2_t> poly;
    poly.reserve(3);

    const float h = nmath_sqrt(3.0f) * 0.5f;
    poly.push_back(make_pt2(-0.5f, -h / 3.0f));
    poly.push_back(make_pt2( 0.5f, -h / 3.0f));
    poly.push_back(make_pt2( 0.0f,  2.0f * h / 3.0f));

    const float c = nmath_cos(-(float)M_PI / 3.0f);
    const float s = nmath_sin(-(float)M_PI / 3.0f);

    for (size_t it = 0; it < iterations; ++it) {
        std::vector<pt2_t> next;
        next.reserve(poly.size() * 4);

        for (size_t i = 0; i < poly.size(); ++i) {
            const pt2_t a = poly[i];
            const pt2_t b = poly[(i + 1) % poly.size()];

            const pt2_t p = make_pt2((2.0f * a.x + b.x) / 3.0f, (2.0f * a.z + b.z) / 3.0f);
            const pt2_t r = make_pt2((a.x + 2.0f * b.x) / 3.0f, (a.z + 2.0f * b.z) / 3.0f);

            const float ex = r.x - p.x;
            const float ez = r.z - p.z;

            const pt2_t q = make_pt2(p.x + ex * c - ez * s, p.z + ex * s + ez * c);

            next.push_back(a);
            next.push_back(p);
            next.push_back(q);
            next.push_back(r);
        }

        poly.swap(next);
    }

    if (area2(poly) < 0.0f) std::reverse(poly.begin(), poly.end());

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    std::vector<int> vi;
    vi.reserve(poly.size());
    for (size_t i = 0; i < poly.size(); ++i) {
        vi.push_back(append_vertex(obj, Vec3(poly[i].x, 0.0f, poly[i].z)));
    }

    std::vector<int> ears;
    ears.reserve(vi.size());
    for (size_t i = 0; i < vi.size(); ++i) ears.push_back((int)i);

    size_t guard = 0;
    while (ears.size() > 2 && guard < poly.size() * poly.size() * 2) {
        bool clipped = false;
        const size_t n = ears.size();

        for (size_t i = 0; i < n; ++i) {
            const int i0 = ears[(i + n - 1) % n];
            const int i1 = ears[i];
            const int i2 = ears[(i + 1) % n];

            const pt2_t &a = poly[(size_t)i0];
            const pt2_t &b = poly[(size_t)i1];
            const pt2_t &cpt = poly[(size_t)i2];

            if (cross2(a, b, cpt) <= 1e-7f) continue;

            bool has_inside = false;
            for (size_t k = 0; k < n; ++k) {
                const int ik = ears[k];
                if (ik == i0 || ik == i1 || ik == i2) continue;
                if (point_in_tri(poly[(size_t)ik], a, b, cpt)) {
                    has_inside = true;
                    break;
                }
            }

            if (has_inside) continue;

            append_triangle(out, vi[(size_t)i0], vi[(size_t)i1], vi[(size_t)i2]);
            ears.erase(ears.begin() + (long)i);
            clipped = true;
            break;
        }

        if (!clipped) break;
        ++guard;
    }
}

    } /* namespace generator */
} /* namespace nmesh */
