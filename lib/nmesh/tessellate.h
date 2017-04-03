#ifndef NMESH_TESSELATE_H_INCLUDED
#define NMESH_TESSELATE_H_INCLUDED

#include <cstdlib>
#include "structs.h"

namespace nmesh {
    namespace generator {

void tessellate(object_t *obj, size_t iterations);
void tessellate(object_t *obj);

    } /* namespace generator */
} /* namespace nmesh */

#endif /* NMESH_TESSELATE_H_INCLUDED */
