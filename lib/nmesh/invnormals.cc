#include "invnormals.h"

namespace nmesh {
    namespace mutator {

void invert_normals(object_t *obj)
{
    if (!obj) return;

    for (size_t i = 0; i < obj->attributes.n.size(); ++i) {
        obj->attributes.n[i] = -obj->attributes.n[i];
    }
}

    } /* namespace mutator */
} /* namespace nmesh */
