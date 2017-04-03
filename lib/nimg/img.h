#ifndef NIMG_IMG_H_INCLUDED
#define NIMG_IMG_H_INCLUDED

#include "pixmap.h"

namespace nimg {
    namespace io {
        namespace load {

int image(const char *filename, Pixmap &map);

        } /* namespace load */


        namespace save {

int png(const char *filename, Pixmap &map);
int bmp(const char *filename, Pixmap &map);
int tga(const char *filename, Pixmap &map);
int hdr(const char *filename, Pixmap &map);

        } /* namespace save */

    } /* namespace io */
} /* namespace nimg */

#endif /* NIMG_IMG_H_INCLUDED */
