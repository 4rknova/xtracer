#ifndef NIMG_CHECKERBOARD_HPP_INCLUDED
#define NIMG_CHECKERBOARD_HPP_INCLUDED

#include "pixmap.h"

namespace NImg {
	namespace Generate {

// RETURN CODES:
// 0. Everything went well.
// 1. Width or Height is 0.
// 2. Failed to initialize the Image.
int checkerboard(Pixmap &map, const unsigned int width, const unsigned int height,
				 ColorRGBAf &a, ColorRGBAf &b);

	} /* namespace Generator */
} /* namespace NImg */

#endif /* NIMG_CHECKERBOARD_HPP_INCLUDED */
