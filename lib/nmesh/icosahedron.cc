#include <nmath/precision.h>
#include <nmath/vector.h>
#include "structs.h"
#include "icosahedron.h"

namespace nmesh {
    namespace generator {

// Ref: http://blog.andreaskahler.com/2009/06/creating-icosphere-mesh-in-code.html
void icosahedron(object_t *obj)
{
    if (!obj) return;

    // create 12 vertices of a icosahedron
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

    std::vector<float> *c = &(obj->attributes.v);

    for (size_t i = 0; i < 12; ++i) {
        c->push_back(v[i].x);
        c->push_back(v[i].y);
        c->push_back(v[i].z);
    }

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

    for (size_t i = 0; i < 60; ++i) {
        nmesh::index_t f;
        f.v  = idx[i];
        obj->shapes[0].mesh.indices.push_back(f);
    }
}

    } /* namespace generator */
} /* namespace nmesh */
