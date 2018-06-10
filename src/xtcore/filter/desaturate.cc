#include <nimg/desaturate.h>
#include "desaturate.h"

namespace xtcore {
    namespace filter {

void Desaturate::render(Pixmap *p)
{
    nimg::filter::desaturate(p);
}

    } /* namespace filter */
} /* namespace xtcore */
