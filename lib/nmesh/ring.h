#ifndef NMESH_RING_H_INCLUDED
#define NMESH_RING_H_INCLUDED

#include "structs.h"

namespace nmesh {
    namespace generator {

void ring(object_t *obj,
          size_t resolution = 48,
          float radius = 1.0f,
          float height = 0.64f,
          float thickness = -1.0f,
          size_t height_resolution = 1);

    } /* namespace generator */
} /* namespace nmesh */

#endif /* NMESH_RING_H_INCLUDED */
