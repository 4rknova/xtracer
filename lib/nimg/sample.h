#ifndef NIMG_SAMPLE_H_INCLUDED
#define NIMG_SAMPLE_H_INCLUDED

#include "color.h"
#include "pixmap.h"

namespace nimg {
    namespace sample {

ColorRGBAf nearest  (const Pixmap &map, double u, double v);
ColorRGBAf bilinear (const Pixmap &map, double u, double v);

    } /* namespace sample */
} /* namespace nimg */

#endif /* NIMG_SAMPLE_H_INCLUDED */
