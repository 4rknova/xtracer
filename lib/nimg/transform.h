#ifndef NIMG_TRANSFORM_H_INCLUDED
#define NIMG_TRANSFORM_H_INCLUDED

#include <cstddef>
#include "pixmap.h"

namespace nimg {
    namespace transform {

bool flip_horizontal (Pixmap *map);
bool flip_vertical   (Pixmap *map);

    } /* namespace transform */
} /* namespace nimg */

#endif /* NIMG_TRANSFORM_H_INCLUDED */
