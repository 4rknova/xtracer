#ifndef NIMG_IMG_H_INCLUDED
#define NIMG_IMG_H_INCLUDED

#include <vector>

#include "pixmap.h"

namespace nimg {
    namespace io {
        namespace load {

int image(const char *filename, Pixmap &map);
int image_memory(const unsigned char *data, size_t size, Pixmap &map);
int exr(const char *filename, Pixmap &map);
int exr_memory(const unsigned char *data, size_t size, Pixmap &map);

        } /* namespace load */

        namespace save {

int exr(const char *filename, Pixmap &map);
int png(const char *filename, Pixmap &map);
int jpg(const char *filename, Pixmap &map);
int bmp(const char *filename, Pixmap &map);
int tga(const char *filename, Pixmap &map);
int hdr(const char *filename, Pixmap &map);

int exr_memory(Pixmap &map, std::vector<unsigned char> &out);
int png_memory(Pixmap &map, std::vector<unsigned char> &out);
int jpg_memory(Pixmap &map, std::vector<unsigned char> &out);
int bmp_memory(Pixmap &map, std::vector<unsigned char> &out);
int tga_memory(Pixmap &map, std::vector<unsigned char> &out);
int hdr_memory(Pixmap &map, std::vector<unsigned char> &out);

        } /* namespace save */

    } /* namespace io */
} /* namespace nimg */

#endif /* NIMG_IMG_H_INCLUDED */
