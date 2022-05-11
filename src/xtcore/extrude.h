#ifndef XTCORE_EXTRUDE_H_INCLUDED
#define XTCORE_EXTRUDE_H_INCLUDED

#include <nmesh/structs.h>
#include "sampler_cubemap.h"

namespace xtcore {
    namespace auxiliary {

void extrude(nmesh::object_t *obj, xtcore::sampler::Cubemap *cb);

    } /* namespace auxiliary */
} /* namespace auxiliary */

#endif /* XTCORE_EXTRUDE_H_INCLUDED */
