#include <nmath/precision.h>
#include <nmath/vector.h>
#include "structs.h"
#include "cone.h"

namespace nmesh {
    namespace generator {


// Ref: https://www.danielsieger.com/blog/2021/05/03/generating-primitive-shapes.html
void cone(object_t *obj, float height, float radius, size_t resolution)
{
    if (!obj) return;

    std::vector<float> *c = &(obj->attributes.v);

    for (size_t i = 0; i < resolution; ++i) {
        float r = float(i) * nmath::PI * 2.0 / resolution;
        float x = nmath_cos(r) * radius;
        float z = nmath_sin(r) * radius;
        c->push_back(x);
        c->push_back(0.0);
        c->push_back(z);
    }

    c->push_back( 0.0);
    c->push_back( 0.0);
    c->push_back( 0.0);

    c->push_back(0.0);
    c->push_back(height);
    c->push_back(0.0);

    nmesh::shape_t shape;
    obj->shapes.push_back(shape);
/*    
     // generate triangular faces
    for (size_t i = 0; i < resolution; ++i) {
        size_t = (i + 1) % resolution;
        f.v = c->size() - 1;
        obj->shapes[0].mesh.indices.push_back(f);


    }
*/
}

    } /* namespace generator */
} /* namespace nmesh */
