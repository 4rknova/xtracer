#ifndef NIMG_CHECKERBOARD_H_INCLUDED
#define NIMG_CHECKERBOARD_H_INCLUDED

#include "pixmap.h"

namespace nimg {
	namespace generator {

// RETURN CODES:
// 0. Everything went well.
// 1. Width or Height is 0.
// 2. Failed to initialize the Image.
int checkerboard(Pixmap &map, const unsigned int width, const unsigned int height,
				 ColorRGBAf &a, ColorRGBAf &b);

	} /* namespace generator */
} /* namespace nimg */

#endif /* NIMG_CHECKERBOARD_H_INCLUDED */
