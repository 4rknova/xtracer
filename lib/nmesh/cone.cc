#include <nmath/precision.h>
#include <nmath/vector.h>
#include "structs.h"
#include "cone.h"

namespace nmesh {
    namespace generator {


// Ref: https://www.danielsieger.com/blog/2021/05/03/generating-primitive-shapes.html
void cone(object_t *obj, float height, float radius, size_t resolution)
{
    if (!obj || resolution < 3) return;

    std::vector<float> *c = &(obj->attributes.v);
    std::vector<float> *n = &(obj->attributes.n);

    // slope factor for side normals: outward component along the slant
    float slope = radius / nmath_sqrt(height * height + radius * radius);
    float ycomp = height / nmath_sqrt(height * height + radius * radius);

    // ring vertices + normals (0 .. resolution-1)
    for (size_t i = 0; i < resolution; ++i) {
        float r = float(i) * nmath::PI * 2.0f / float(resolution);
        float cx = nmath_cos(r);
        float cz = nmath_sin(r);
        c->push_back(cx * radius);
        c->push_back(0.0f);
        c->push_back(cz * radius);
        n->push_back(cx * slope);
        n->push_back(ycomp);
        n->push_back(cz * slope);
    }

    // bottom center vertex + normal (index: resolution)
    c->push_back(0.0f); c->push_back(0.0f); c->push_back(0.0f);
    n->push_back(0.0f); n->push_back(-1.0f); n->push_back(0.0f);

    // apex vertex + normal (index: resolution + 1)
    c->push_back(0.0f); c->push_back(height); c->push_back(0.0f);
    n->push_back(0.0f); n->push_back(1.0f); n->push_back(0.0f);

    nmesh::shape_t shape;
    obj->shapes.push_back(shape);

    size_t bot = resolution;
    size_t apex = resolution + 1;

    for (size_t i = 0; i < resolution; ++i) {
        size_t j = (i + 1) % resolution;

        // bottom cap (CCW from below → normal points down)
        nmesh::index_t a, b, bc;
        a.v = (int)bot;  a.n = (int)bot;  a.uv = -1;
        b.v = (int)j;    b.n = (int)j;    b.uv = -1;
        bc.v = (int)i;   bc.n = (int)i;   bc.uv = -1;
        obj->shapes[0].mesh.indices.push_back(a);
        obj->shapes[0].mesh.indices.push_back(b);
        obj->shapes[0].mesh.indices.push_back(bc);

        // side face
        nmesh::index_t s0, s1, s2;
        s0.v = (int)apex; s0.n = (int)apex; s0.uv = -1;
        s1.v = (int)i;    s1.n = (int)i;    s1.uv = -1;
        s2.v = (int)j;    s2.n = (int)j;    s2.uv = -1;
        obj->shapes[0].mesh.indices.push_back(s0);
        obj->shapes[0].mesh.indices.push_back(s1);
        obj->shapes[0].mesh.indices.push_back(s2);
    }
}

    } /* namespace generator */
} /* namespace nmesh */
