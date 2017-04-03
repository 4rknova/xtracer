#ifndef NIMG_SOLIDCOL_H_INCLUDED
#define NIMG_SOLIDCOL_H_INCLUDED

#include "pixmap.h"

namespace nimg {
    namespace generator {

int solid_color(Pixmap &map, const unsigned int width, const unsigned int height, ColorRGBAf &col);

    } /* namespace generator */
} /* namespace nimg */

#endif /* NIMG_SOLIDCOL_H_INCLUDED */
