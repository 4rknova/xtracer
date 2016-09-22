#ifndef NIMG_SOLIDCOL_H_INCLUDED
#define NIMG_SOLIDCOL_H_INCLUDED

#include "pixmap.h"

namespace NImg {
    namespace Generate {

int solid_color(Pixmap &map, const unsigned int width, const unsigned int height, ColorRGBAf &col);

    } /* namespace Generate */
} /* namespace NImg */

#endif /* NIMG_SOLIDCOL_H_INCLUDED */
