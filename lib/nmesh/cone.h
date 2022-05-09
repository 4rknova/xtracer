#ifndef NMESH_CONE_H_INCLUDED
#define NMESH_CONE_H_INCLUDED

#include "structs.h"

namespace nmesh {
    namespace generator {

void cone(object_t *obj, float height, float radius, size_t resolution = 3);

    } /* namespace generator */
} /* namespace nmesh */

#endif /* NMESH_CONE_H_INCLUDED */
