#include <nmath/precision.h>
#include <nmath/vector.h>
#include "structs.h"
#include "plane.h"

namespace nmesh {
    namespace generator {


// Ref: https://www.danielsieger.com/blog/2021/05/03/generating-primitive-shapes.html
void plane(object_t *obj, size_t resolution)
{
    if (!obj) return;

    std::vector<float> *c = &(obj->attributes.v);
    std::vector<float> *n = &(obj->attributes.n);
    std::vector<float> *t = &(obj->attributes.uv);

    NMath::Vector3f p;
    p.x = -0.5;
    p.y =  0.0;
    p.z = -0.5;

    for (size_t i = 0; i < resolution + 1; ++i) {
        for (size_t j = 0; j < resolution + 1; ++j) {
            c->push_back(p.x);
            c->push_back(0.0);
            c->push_back(p.z);
            n->push_back(0.0);
            n->push_back(1.0);
            n->push_back(0.0);
            t->push_back(p.x + 0.5);
            t->push_back(p.z + 0.5);
            p.x += 1.0 / resolution;
        }
        p.x = -0.5;
        p.z += 1.0 / resolution;
    }

    nmesh::shape_t shape;
    obj->shapes.push_back(shape);

    for (size_t i = 0; i < resolution; ++i) {
        for (size_t j = 0; j < resolution; ++j) {
            nmesh::index_t f;

            size_t v0 = j + i * (resolution + 1);
            size_t v1 = v0 + resolution + 1;
            size_t v2 = v0 + resolution + 2;
            size_t v3 = v0 + 1;

            f.v = v0; f.n = v0; f.uv = v0;
            obj->shapes[0].mesh.indices.push_back(f);

            f.v = v1; f.n = v1; f.uv = v1;
            obj->shapes[0].mesh.indices.push_back(f);
            
            f.v = v2; f.n = v2; f.uv = v2;
            obj->shapes[0].mesh.indices.push_back(f);

            f.v = v0; f.n = v0; f.uv = v0;
            obj->shapes[0].mesh.indices.push_back(f);
     
            f.v = v2; f.n = v2; f.uv = v2;
            obj->shapes[0].mesh.indices.push_back(f);
          
            f.v = v3; f.n = v3; f.uv = v3;
            obj->shapes[0].mesh.indices.push_back(f);
        }
    }
}

    } /* namespace generator */
} /* namespace nmesh */
