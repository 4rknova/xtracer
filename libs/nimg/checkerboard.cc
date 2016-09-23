#include "color.h"
#include "checkerboard.h"

namespace nimg {
	namespace generator {

int checkerboard(Pixmap &map, const unsigned int width, const unsigned int height,
				 ColorRGBAf &a, ColorRGBAf &b)
{
	if (!width || !height) return 1;

	if (map.init(width, height)) return 2;

	for (unsigned int j = 0; j < map.height(); ++j) {
		for (unsigned int i = 0; i < map.width(); ++i) {

			bool y = (j < map.height() / 2);
			bool x = (i < map.width()  / 2);

			ColorRGBAf val = (x ^ y ? a : b);
			ColorRGBAf &pixel = map.pixel(i, j);

			pixel.r(val.r());
			pixel.g(val.g());
			pixel.b(val.b());
			pixel.a(val.r());
		}
	}

	return 0;
}

	} /* namespace generator */
} /* namespace nimg */
