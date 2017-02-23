#include <nmath/precision.h>
#include "icosahedron.h"

namespace nmesh {
    namespace generator {

// http://blog.andreaskahler.com/2009/06/creating-icosphere-mesh-in-code.html

void icosahedron(NMesh::object_t *obj)
{
    if (!obj) return;

    NMesh::shape_t shape;
    obj->shapes.push_back(shape);

    // create 12 vertices of a icosahedron
    float t = (1.f + nmath_sqrt(5.f)) * 0.5f;
    // TO DO: Each vertex needs to also be normalized
    obj->attributes.v.push_back(-1); obj->attributes.v.push_back( t); obj->attributes.v.push_back( 0);
    obj->attributes.v.push_back( 1); obj->attributes.v.push_back( t); obj->attributes.v.push_back( 0);
    obj->attributes.v.push_back(-1); obj->attributes.v.push_back(-t); obj->attributes.v.push_back( 0);
    obj->attributes.v.push_back( 1); obj->attributes.v.push_back(-t); obj->attributes.v.push_back( 0);
    obj->attributes.v.push_back( 0); obj->attributes.v.push_back(-1); obj->attributes.v.push_back( t);
    obj->attributes.v.push_back( 0); obj->attributes.v.push_back( 1); obj->attributes.v.push_back( t);
    obj->attributes.v.push_back( 0); obj->attributes.v.push_back(-1); obj->attributes.v.push_back(-t);
    obj->attributes.v.push_back( 0); obj->attributes.v.push_back( 1); obj->attributes.v.push_back(-t);
    obj->attributes.v.push_back( t); obj->attributes.v.push_back( 0); obj->attributes.v.push_back(-1);
    obj->attributes.v.push_back( t); obj->attributes.v.push_back( 0); obj->attributes.v.push_back( 1);
    obj->attributes.v.push_back(-t); obj->attributes.v.push_back( 0); obj->attributes.v.push_back(-1);
    obj->attributes.v.push_back(-t); obj->attributes.v.push_back( 0); obj->attributes.v.push_back( 1);


    NMesh::index_t i;

    i.v =  0; obj->shapes[0].mesh.indices.push_back(i);
    i.v = 11; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  5; obj->shapes[0].mesh.indices.push_back(i);

    i.v =  0; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  5; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  1; obj->shapes[0].mesh.indices.push_back(i);

    i.v =  0; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  1; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  7; obj->shapes[0].mesh.indices.push_back(i);

    i.v =  0; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  7; obj->shapes[0].mesh.indices.push_back(i);
    i.v = 10; obj->shapes[0].mesh.indices.push_back(i);

    i.v =  0; obj->shapes[0].mesh.indices.push_back(i);
    i.v = 10; obj->shapes[0].mesh.indices.push_back(i);
    i.v = 11; obj->shapes[0].mesh.indices.push_back(i);

    i.v =  1; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  5; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  9; obj->shapes[0].mesh.indices.push_back(i);

    i.v =  5; obj->shapes[0].mesh.indices.push_back(i);
    i.v = 11; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  4; obj->shapes[0].mesh.indices.push_back(i);

    i.v = 11; obj->shapes[0].mesh.indices.push_back(i);
    i.v = 10; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  2; obj->shapes[0].mesh.indices.push_back(i);

    i.v = 10; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  7; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  6; obj->shapes[0].mesh.indices.push_back(i);

    i.v =  7; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  1; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  8; obj->shapes[0].mesh.indices.push_back(i);

    i.v =  3; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  9; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  4; obj->shapes[0].mesh.indices.push_back(i);

    i.v =  3; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  4; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  2; obj->shapes[0].mesh.indices.push_back(i);

    i.v =  3; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  2; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  6; obj->shapes[0].mesh.indices.push_back(i);

    i.v =  3; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  6; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  8; obj->shapes[0].mesh.indices.push_back(i);

    i.v =  3; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  8; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  9; obj->shapes[0].mesh.indices.push_back(i);

    i.v =  4; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  9; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  5; obj->shapes[0].mesh.indices.push_back(i);

    i.v =  2; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  4; obj->shapes[0].mesh.indices.push_back(i);
    i.v = 11; obj->shapes[0].mesh.indices.push_back(i);

    i.v =  6; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  2; obj->shapes[0].mesh.indices.push_back(i);
    i.v = 10; obj->shapes[0].mesh.indices.push_back(i);

    i.v =  8; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  6; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  7; obj->shapes[0].mesh.indices.push_back(i);

    i.v =  9; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  8; obj->shapes[0].mesh.indices.push_back(i);
    i.v =  1; obj->shapes[0].mesh.indices.push_back(i);
}

    } /* namespace generator */
} /* namespace nmesh */
