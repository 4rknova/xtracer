#include <nmath/precision.h>
#include <nmath/vector.h>

#include "ring.h"

namespace nmesh {
    namespace generator {

namespace {

static void append_vertex(object_t *obj, const nmath::Vector3f &p, const nmath::Vector3f &n, float u, float v)
{
    obj->attributes.v.push_back(p.x);
    obj->attributes.v.push_back(p.y);
    obj->attributes.v.push_back(p.z);

    obj->attributes.n.push_back(n.x);
    obj->attributes.n.push_back(n.y);
    obj->attributes.n.push_back(n.z);

    obj->attributes.uv.push_back(u);
    obj->attributes.uv.push_back(v);
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

void ring(object_t *obj, size_t resolution, float radius, float height, float thickness, size_t height_resolution)
{
    if (!obj) return;

    const size_t seg = (resolution < 24 ? 24 : resolution);
    const size_t hseg = (height_resolution < 1 ? 1 : height_resolution);
    const float outer_r = (radius > 0.0f ? radius : 1.0f);
    const float default_thickness = outer_r * 0.16f;
    const float ring_thickness = (thickness > 0.0f ? thickness : default_thickness);
    const float clamped_thickness = (ring_thickness >= outer_r ? (outer_r - 0.001f) : ring_thickness);
    const float inner_r = (clamped_thickness > 0.0f ? (outer_r - clamped_thickness) : outer_r * 0.999f);
    const float h = (height > 0.0f ? height : 0.64f);
    const float half_h = h * 0.5f;

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    const size_t base_outer = 0;
    for (size_t y = 0; y <= hseg; ++y) {
        const float v = (float)y / (float)hseg;
        const float py = -half_h + h * v;
        for (size_t i = 0; i <= seg; ++i) {
            const float u = (float)i / (float)seg;
            const float a = (float)(nmath::PI_DOUBLE * 2.0) * u;
            const float ca = nmath_cos(a);
            const float sa = nmath_sin(a);

            const nmath::Vector3f n(ca, 0.0f, sa);
            append_vertex(obj, nmath::Vector3f(outer_r * ca, py, outer_r * sa), n, u, v);
        }
    }

    const size_t base_inner = obj->attributes.v.size() / 3;
    for (size_t y = 0; y <= hseg; ++y) {
        const float v = (float)y / (float)hseg;
        const float py = -half_h + h * v;
        for (size_t i = 0; i <= seg; ++i) {
            const float u = (float)i / (float)seg;
            const float a = (float)(nmath::PI_DOUBLE * 2.0) * u;
            const float ca = nmath_cos(a);
            const float sa = nmath_sin(a);

            const nmath::Vector3f n(-ca, 0.0f, -sa);
            append_vertex(obj, nmath::Vector3f(inner_r * ca, py, inner_r * sa), n, u, v);
        }
    }

    const size_t base_top = obj->attributes.v.size() / 3;
    for (size_t i = 0; i <= seg; ++i) {
        const float u = (float)i / (float)seg;
        const float a = (float)(nmath::PI_DOUBLE * 2.0) * u;
        const float ca = nmath_cos(a);
        const float sa = nmath_sin(a);

        append_vertex(obj, nmath::Vector3f(inner_r * ca, half_h, inner_r * sa), nmath::Vector3f(0, 1, 0), u, 0.0f);
        append_vertex(obj, nmath::Vector3f(outer_r * ca, half_h, outer_r * sa), nmath::Vector3f(0, 1, 0), u, 1.0f);
    }

    const size_t base_bottom = obj->attributes.v.size() / 3;
    for (size_t i = 0; i <= seg; ++i) {
        const float u = (float)i / (float)seg;
        const float a = (float)(nmath::PI_DOUBLE * 2.0) * u;
        const float ca = nmath_cos(a);
        const float sa = nmath_sin(a);

        append_vertex(obj, nmath::Vector3f(inner_r * ca, -half_h, inner_r * sa), nmath::Vector3f(0, -1, 0), u, 0.0f);
        append_vertex(obj, nmath::Vector3f(outer_r * ca, -half_h, outer_r * sa), nmath::Vector3f(0, -1, 0), u, 1.0f);
    }

    for (size_t y = 0; y < hseg; ++y) {
        for (size_t i = 0; i < seg; ++i) {
            const int o0 = (int)(base_outer + y * (seg + 1) + i);
            const int o1 = o0 + 1;
            const int o2 = o0 + (int)(seg + 1);
            const int o3 = o2 + 1;

            append_triangle(out, o0, o1, o3);
            append_triangle(out, o0, o3, o2);

            const int in0 = (int)(base_inner + y * (seg + 1) + i);
            const int in1 = in0 + 1;
            const int in2 = in0 + (int)(seg + 1);
            const int in3 = in2 + 1;

            append_triangle(out, in0, in3, in1);
            append_triangle(out, in0, in2, in3);
        }
    }

    for (size_t i = 0; i < seg; ++i) {

        const int t0 = (int)(base_top + i * 2);
        const int t1 = t0 + 1;
        const int t2 = t0 + 2;
        const int t3 = t0 + 3;

        append_triangle(out, t0, t2, t3);
        append_triangle(out, t0, t3, t1);

        const int b0 = (int)(base_bottom + i * 2);
        const int b1 = b0 + 1;
        const int b2 = b0 + 2;
        const int b3 = b0 + 3;

        append_triangle(out, b0, b3, b2);
        append_triangle(out, b0, b1, b3);
    }
}

    } /* namespace generator */
} /* namespace nmesh */
