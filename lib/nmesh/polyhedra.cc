#include <algorithm>
#include <cmath>
#include <map>
#include <vector>

#include <nmath/precision.h>
#include <nmath/vector.h>

#include "polyhedra.h"

namespace nmesh {
    namespace generator {

namespace {

typedef nmath::Vector3f Vec3;

static int append_vertex(object_t *obj, const Vec3 &p, const Vec3 &n)
{
    const int idx = (int)(obj->attributes.v.size() / 3);
    obj->attributes.v.push_back(p.x);
    obj->attributes.v.push_back(p.y);
    obj->attributes.v.push_back(p.z);
    obj->attributes.n.push_back(n.x);
    obj->attributes.n.push_back(n.y);
    obj->attributes.n.push_back(n.z);
    return idx;
}

static void append_triangle(shape_t &shape, int a, int b, int c)
{
    index_t ia;
    ia.v = a; ia.n = a; ia.uv = -1;
    index_t ib;
    ib.v = b; ib.n = b; ib.uv = -1;
    index_t ic;
    ic.v = c; ic.n = c; ic.uv = -1;

    shape.mesh.indices.push_back(ia);
    shape.mesh.indices.push_back(ib);
    shape.mesh.indices.push_back(ic);
}

} /* namespace */

void tetrahedron(object_t *obj)
{
    if (!obj) return;

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    const float s = 1.0f / nmath_sqrt(3.0f);
    const Vec3 vertices[4] = {
        Vec3( s,  s,  s),
        Vec3(-s, -s,  s),
        Vec3(-s,  s, -s),
        Vec3( s, -s, -s)
    };

    int vi[4];
    for (int i = 0; i < 4; ++i) vi[i] = append_vertex(obj, vertices[i], vertices[i].normalized());

    const int idx[] = {
        0, 1, 2,
        0, 3, 1,
        0, 2, 3,
        1, 3, 2
    };

    for (size_t i = 0; i < sizeof(idx) / sizeof(idx[0]); i += 3) {
        append_triangle(out, vi[idx[i]], vi[idx[i + 1]], vi[idx[i + 2]]);
    }
}

void cube(object_t *obj)
{
    if (!obj) return;

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    const Vec3 vertices[8] = {
        Vec3(-0.5f, -0.5f, -0.5f),
        Vec3( 0.5f, -0.5f, -0.5f),
        Vec3( 0.5f,  0.5f, -0.5f),
        Vec3(-0.5f,  0.5f, -0.5f),
        Vec3(-0.5f, -0.5f,  0.5f),
        Vec3( 0.5f, -0.5f,  0.5f),
        Vec3( 0.5f,  0.5f,  0.5f),
        Vec3(-0.5f,  0.5f,  0.5f)
    };

    int vi[8];
    for (int i = 0; i < 8; ++i) vi[i] = append_vertex(obj, vertices[i], vertices[i].normalized());

    const int idx[] = {
        0, 2, 1, 0, 3, 2, // -Z
        4, 5, 6, 4, 6, 7, // +Z
        0, 1, 5, 0, 5, 4, // -Y
        3, 7, 6, 3, 6, 2, // +Y
        1, 2, 6, 1, 6, 5, // +X
        0, 4, 7, 0, 7, 3  // -X
    };

    for (size_t i = 0; i < sizeof(idx) / sizeof(idx[0]); i += 3) {
        append_triangle(out, vi[idx[i]], vi[idx[i + 1]], vi[idx[i + 2]]);
    }
}

void octahedron(object_t *obj)
{
    if (!obj) return;

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    const Vec3 vertices[6] = {
        Vec3( 1.0f,  0.0f,  0.0f),
        Vec3(-1.0f,  0.0f,  0.0f),
        Vec3( 0.0f,  1.0f,  0.0f),
        Vec3( 0.0f, -1.0f,  0.0f),
        Vec3( 0.0f,  0.0f,  1.0f),
        Vec3( 0.0f,  0.0f, -1.0f)
    };

    int vi[6];
    for (int i = 0; i < 6; ++i) vi[i] = append_vertex(obj, vertices[i], vertices[i].normalized());

    const int idx[] = {
        2, 0, 4,
        2, 4, 1,
        2, 1, 5,
        2, 5, 0,
        3, 4, 0,
        3, 1, 4,
        3, 5, 1,
        3, 0, 5
    };

    for (size_t i = 0; i < sizeof(idx) / sizeof(idx[0]); i += 3) {
        append_triangle(out, vi[idx[i]], vi[idx[i + 1]], vi[idx[i + 2]]);
    }
}

void dodecahedron(object_t *obj)
{
    if (!obj) return;

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    // Build a dodecahedron as the dual polyhedron of the icosahedron.
    const float k = (1.0f + nmath_sqrt(5.0f)) * 0.5f;
    const Vec3 ico_vertices[12] = {
        Vec3(-1,  k,  0).normalized(),
        Vec3( 1,  k,  0).normalized(),
        Vec3(-1, -k,  0).normalized(),
        Vec3( 1, -k,  0).normalized(),
        Vec3( 0, -1,  k).normalized(),
        Vec3( 0,  1,  k).normalized(),
        Vec3( 0, -1, -k).normalized(),
        Vec3( 0,  1, -k).normalized(),
        Vec3( k,  0, -1).normalized(),
        Vec3( k,  0,  1).normalized(),
        Vec3(-k,  0, -1).normalized(),
        Vec3(-k,  0,  1).normalized()
    };

    const int ico_faces[20][3] = {
        { 0,11, 5}, { 0, 5, 1}, { 0, 1, 7}, { 0, 7,10}, { 0,10,11},
        { 1, 5, 9}, { 5,11, 4}, {11,10, 2}, {10, 7, 6}, { 7, 1, 8},
        { 3, 9, 4}, { 3, 4, 2}, { 3, 2, 6}, { 3, 6, 8}, { 3, 8, 9},
        { 4, 9, 5}, { 2, 4,11}, { 6, 2,10}, { 8, 6, 7}, { 9, 8, 1}
    };

    std::vector<Vec3> dodeca_vertices;
    dodeca_vertices.reserve(20);
    for (int i = 0; i < 20; ++i) {
        Vec3 c = ico_vertices[ico_faces[i][0]] + ico_vertices[ico_faces[i][1]] + ico_vertices[ico_faces[i][2]];
        dodeca_vertices.push_back(c.normalized());
    }

    std::vector<int> dv;
    dv.reserve(dodeca_vertices.size());
    for (size_t i = 0; i < dodeca_vertices.size(); ++i) {
        dv.push_back(append_vertex(obj, dodeca_vertices[i], dodeca_vertices[i]));
    }

    std::map<int, std::vector<int> > adjacency;
    for (int face_idx = 0; face_idx < 20; ++face_idx) {
        for (int j = 0; j < 3; ++j) adjacency[ico_faces[face_idx][j]].push_back(face_idx);
    }

    for (int vi = 0; vi < 12; ++vi) {
        std::vector<int> ring = adjacency[vi];
        if (ring.size() != 5) continue;

        const Vec3 axis = ico_vertices[vi].normalized();
        Vec3 u = nmath::cross(axis, Vec3(0.0f, 1.0f, 0.0f));
        if (u.length() < 1e-6f) u = nmath::cross(axis, Vec3(1.0f, 0.0f, 0.0f));
        u = u.normalized();
        const Vec3 v = nmath::cross(axis, u).normalized();

        std::vector<std::pair<float, int> > ordered;
        ordered.reserve(ring.size());
        for (size_t i = 0; i < ring.size(); ++i) {
            const Vec3 p = dodeca_vertices[ring[i]];
            Vec3 d = p - axis * nmath::dot(p, axis);
            if (d.length() > 1e-6f) d = d.normalized();
            const float a = nmath_atan2(nmath::dot(d, v), nmath::dot(d, u));
            ordered.push_back(std::make_pair(a, ring[i]));
        }

        std::sort(ordered.begin(), ordered.end());
        for (size_t i = 1; i + 1 < ordered.size(); ++i) {
            append_triangle(out, dv[ordered[0].second], dv[ordered[i].second], dv[ordered[i + 1].second]);
        }
    }
}

    } /* namespace generator */
} /* namespace nmesh */
