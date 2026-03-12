#ifndef NMESH_EXTRAS_H_INCLUDED
#define NMESH_EXTRAS_H_INCLUDED

#include "structs.h"

namespace nmesh {
    namespace generator {

void capsule(object_t *obj, size_t resolution = 32);
void cylinder(object_t *obj, size_t resolution = 32);
void capped_cylinder(object_t *obj, size_t resolution = 32);
void cone(object_t *obj, size_t resolution = 32);
void truncated_cone(object_t *obj, size_t resolution = 32);
void torus_knot(object_t *obj, size_t resolution = 48);
void icosphere(object_t *obj, size_t resolution = 32);
void geodesic_dome(object_t *obj, size_t resolution = 32);
void icosa_cage(object_t *obj, size_t resolution = 32);
void menger_sponge(object_t *obj, size_t resolution = 2);
void sierpinski_tetrahedron(object_t *obj, size_t resolution = 2);
void mobius_strip(object_t *obj, size_t resolution = 64);
void klein_bottle(object_t *obj, size_t resolution = 64);

    } /* namespace generator */
} /* namespace nmesh */

#endif /* NMESH_EXTRAS_H_INCLUDED */
