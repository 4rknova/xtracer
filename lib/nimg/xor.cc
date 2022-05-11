#include "color.h"
#include "xor.h"

namespace nimg {
	namespace generator {

int xortex(Pixmap &map, const unsigned int width, const unsigned int height)
{
	if (!width || !height)
		return 1;

	if(map.init(width, height))
		return 2;

	for(unsigned int j = 0; j < map.height(); j++) {
		for(unsigned int i = 0; i < map.width(); i++) {
			float val = (float)(i ^ j) / 255.0f;
			ColorRGBAf &pixel = map.pixel(i, j);

			pixel.r(val);
			pixel.g(val);
			pixel.b(val);
			pixel.a(1.0f);
		}
	}

	return 0;
}

	} /* namespace generator */
} /* namespace nimg */
