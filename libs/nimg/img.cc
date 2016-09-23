#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "img.h"

namespace nimg {
    namespace io {
        namespace load {

int image(const char *filename, Pixmap &map)
{
    if (!filename) return 1;

    int w, h, bpp;
    unsigned char *data = stbi_load(filename, &w, &h, &bpp, STBI_rgb);

    if (data == NULL) return 2;

    map.init(w,h);

    int res = 0;

    if (bpp == 3) {
        for (size_t x = 0; x < (size_t)w; ++x) {
            for (size_t y = 0; y < (size_t)h; ++y) {
                size_t i = 3 * (y * w + x);

                ColorRGBAf pixel;
                pixel.r(data[i  ] / 255.f);
                pixel.g(data[i+1] / 255.f);
                pixel.b(data[i+2] / 255.f);

                map.pixel(x, y) = pixel;
            }
        }
    }
    else  res = -1; // incompatible format

    stbi_image_free(data);

    return res;
}

        } /* namespace load */
    } /* namespace io */
} /* namespace nimg */
