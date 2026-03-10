#ifndef NMESH_POLYHEDRA_H_INCLUDED
#define NMESH_POLYHEDRA_H_INCLUDED

#include "structs.h"

namespace nmesh {
    namespace generator {

void tetrahedron(object_t *obj);
void cube(object_t *obj);
void octahedron(object_t *obj);
void dodecahedron(object_t *obj);

    } /* namespace generator */
} /* namespace nmesh */

#endif /* NMESH_POLYHEDRA_H_INCLUDED */
