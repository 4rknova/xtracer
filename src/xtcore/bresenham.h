#ifndef XTCORE_BRESENHAM_H_INCLUDED
#define XTCORE_BRESENHAM_H_INCLUDED

#include <nimg/color.h>
#include "tile.h"

namespace xtcore {
    namespace render {

void bresenham(int x0, int y0, int x1, int y1, nimg::ColorRGBAf &color, tile_t &tile);

    } /* namespace render */
} /* namespace xtcore */

#endif /* XTCORE_BRESENHAM_H_INCLUDED */
