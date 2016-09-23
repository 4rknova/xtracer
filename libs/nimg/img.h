#ifndef NIMG_IMG_H_INCLUDED
#define NIMG_IMG_H_INCLUDED

#include "pixmap.h"

namespace nimg {
    namespace io {
        namespace load {

int image(const char *filename, Pixmap &map);

        } /* namespace load */
    } /* namespace io */
} /* namespace nimg */

#endif /* NIMG_IMG_H_INCLUDED */
