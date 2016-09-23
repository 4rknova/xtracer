#ifndef NIMG_XOR_H_INCLUDED
#define NIMG_XOR_H_INCLUDED

#include "pixmap.h"

namespace nimg {
	namespace generator {

// RETURN CODES:
// 0. Everything went well.
// 1. Width or Height is 0.
// 2. Failed to initialize the Image.
int xortex(Pixmap &map, const unsigned int width, const unsigned int height);

	} /* namespace generator */
} /* namespace nimg */

#endif /* NIMG_XOR_H_INCLUDED */
