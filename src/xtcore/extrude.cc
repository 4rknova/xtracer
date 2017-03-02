#include <cstddef>
#include <nmath/vector.h>
#include <nimg/color.h>
#include "extrude.h"

namespace xtracer {
    namespace auxiliary {

void extrude(nmesh::object_t *obj, xtracer::assets::Cubemap *cb)
{
    if (!obj || !cb) return;

    for (size_t i = 0; i < obj->attributes.v.size(); i+=3) {
        NMath::Vector3f pos(obj->attributes.v[i  ]
                          , obj->attributes.v[i+1]
                          , obj->attributes.v[i+2]);

        pos.normalize();

        nimg::ColorRGBf c = cb->sample(pos);

        pos = pos * c.r();

        obj->attributes.v[i  ] = pos.x;
        obj->attributes.v[i+1] = pos.y;
        obj->attributes.v[i+2] = pos.z;
    }
    printf("\n done");
}

    } /* namespace auxiliary */
} /* namespace auxiliary */
