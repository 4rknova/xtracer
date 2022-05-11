#include <cstddef>
#include <nmath/vector.h>
#include <nimg/color.h>
#include "extrude.h"

namespace xtcore {
    namespace auxiliary {

void extrude(nmesh::object_t *obj, xtcore::sampler::Cubemap *cb)
{
    if (!obj || !cb) return;

    for (size_t i = 0; i < obj->attributes.v.size(); i+=3) {
        nmath::Vector3f pos(obj->attributes.v[i  ]
                          , obj->attributes.v[i+1]
                          , obj->attributes.v[i+2]);

        nmath::Vector3f dir = pos.normalized();
        nimg::ColorRGBf c = cb->sample(pos);
        nmath::Vector3f p = 10. * dir * c.r();

        obj->attributes.v[i  ] = p.x;
        obj->attributes.v[i+1] = p.y;
        obj->attributes.v[i+2] = p.z;
    }
}

    } /* namespace auxiliary */
} /* namespace auxiliary */
