#ifndef XTCORE_EMITTER_H_INCLUDED
#define XTCORE_EMITTER_H_INCLUDED

#include <nmath/vector.h>
#include <nimg/color.h>

namespace xtcore {
    namespace asset {

typedef struct {
    NMath::Vector3f position;
    nimg::ColorRGBf intensity;

} emitter_t;

    } /* namespace asset */
} /* namespace xtcore */

#endif /* XTCORE_EMITTER_H_INCLUDED */
