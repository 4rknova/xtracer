#include <nmath/precision.h>
#include <nmath/vector.h>
#include "structs.h"
#include "icosahedron.h"

namespace nmesh {
    namespace generator {

// Ref: http://blog.andreaskahler.com/2009/06/creating-icosphere-mesh-in-code.html
void icosahedron(object_t *obj, bool smooth_normals)
{
    if (!obj) return;

    float k = (1.f + nmath_sqrt(5.f)) / 2.f;

    nmath::Vector3f v[12];
    v[ 0] = nmath::Vector3f(-1, k, 0).normalized();
    v[ 1] = nmath::Vector3f(1, k, 0).normalized();
    v[ 2] = nmath::Vector3f(-1, -k, 0).normalized();
    v[ 3] = nmath::Vector3f(1, -k, 0).normalized();
    v[ 4] = nmath::Vector3f(0, -1, k).normalized();
    v[ 5] = nmath::Vector3f(0, 1, k).normalized();
    v[ 6] = nmath::Vector3f(0, -1, -k).normalized();
    v[ 7] = nmath::Vector3f(0, 1, -k).normalized();
    v[ 8] = nmath::Vector3f(k, 0, -1).normalized();
    v[ 9] = nmath::Vector3f(k, 0, 1).normalized();
    v[10] = nmath::Vector3f(-k, 0, -1).normalized();
    v[11] = nmath::Vector3f(-k, 0, 1).normalized();

    const size_t idx[] = {
           0, 11,  5
         , 0,  5,  1
         , 0,  1,  7
         , 0,  7, 10
         , 0, 10, 11
         , 1,  5,  9
         , 5, 11,  4
         ,11, 10,  2
         ,10,  7,  6
         , 7,  1,  8
         , 3,  9,  4
         , 3,  4,  2
         , 3,  2,  6
         , 3,  6,  8
         , 3,  8,  9
         , 4,  9,  5
         , 2,  4, 11
         , 6,  2, 10
         , 8,  6,  7
         , 9,  8,  1
    };

    nmesh::shape_t shape;
    obj->shapes.push_back(shape);

    std::vector<float> *c = &(obj->attributes.v);
    std::vector<float> *n = &(obj->attributes.n);

    if (smooth_normals) {
        for (size_t i = 0; i < 12; ++i) {
            c->push_back(v[i].x); c->push_back(v[i].y); c->push_back(v[i].z);
            n->push_back(v[i].x); n->push_back(v[i].y); n->push_back(v[i].z);
        }
        for (size_t i = 0; i < 60; ++i) {
            nmesh::index_t f;
            f.v = idx[i]; f.n = idx[i]; f.uv = -1;
            obj->shapes[0].mesh.indices.push_back(f);
        }
    } else {
        for (size_t i = 0; i < 60; i += 3) {
            const nmath::Vector3f &a = v[idx[i]];
            const nmath::Vector3f &b = v[idx[i + 1]];
            const nmath::Vector3f &c_v = v[idx[i + 2]];
            nmath::Vector3f fn = nmath::cross(b - a, c_v - a).normalized();
            const int base = (int)(c->size() / 3);
            for (int j = 0; j < 3; ++j) {
                const nmath::Vector3f &p = v[idx[i + j]];
                c->push_back(p.x); c->push_back(p.y); c->push_back(p.z);
                n->push_back(fn.x); n->push_back(fn.y); n->push_back(fn.z);
            }
            nmesh::index_t fa, fb, fc;
            fa.v = base;     fa.n = base;     fa.uv = -1;
            fb.v = base + 1; fb.n = base + 1; fb.uv = -1;
            fc.v = base + 2; fc.n = base + 2; fc.uv = -1;
            obj->shapes[0].mesh.indices.push_back(fa);
            obj->shapes[0].mesh.indices.push_back(fb);
            obj->shapes[0].mesh.indices.push_back(fc);
        }
    }
}

    } /* namespace generator */
} /* namespace nmesh */
