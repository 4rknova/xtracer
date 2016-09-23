#include "solidcol.h"

namespace nimg {
    namespace generator {

int solid_color(Pixmap &map, const unsigned int width, const unsigned int height, ColorRGBAf &col)
{
    if (!width || ! height) return 1;

    if (map.init(width, height)) return 2;

    for (unsigned int j = 0; j < map.height(); ++j) {
        for (unsigned int i = 0; i < map.width(); ++i) {

            ColorRGBAf &pixel = map.pixel(i, j);
            pixel.r(col.r());
            pixel.g(col.g());
            pixel.b(col.b());
            pixel.a(col.a());
        }
    }

    return 0;
}

    } /* namespace generator */
} /* namespace nimg */
