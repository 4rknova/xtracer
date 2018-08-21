#include "luminance.h"
#include "desaturate.h"
#include <cstdio>
namespace nimg {
	namespace filter {

void desaturate(Pixmap *p)
{
    if (!p) return;

    for (size_t y = 0; y < p->height(); ++y) {
        for (size_t x = 0; x < p->width(); ++x) {
            ColorRGBf col = p->pixel_ro(x, y);
            float val = nimg::eval::luminance(col);
            p->pixel(x, y) = ColorRGBf(val, val, val);
        }
    }
}

	} /* namespace filter */
} /* namespace nimg */
