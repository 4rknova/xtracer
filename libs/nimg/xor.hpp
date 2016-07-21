#ifndef NIMG_XOR_HPP_INCLUDED
#define NIMG_XOR_HPP_INCLUDED

#include "pixmap.h"

namespace NImg {
	namespace Generate {

// RETURN CODES:
// 0. Everything went well.
// 1. Width or Height is 0.
// 2. Failed to initialize the Image.
int xortex(Pixmap &map, const unsigned int width, const unsigned int height);

	} /* namespace Generator */
} /* namespace NImg */

#endif /* NIMG_XOR_HPP_INCLUDED */
