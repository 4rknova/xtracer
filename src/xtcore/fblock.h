#ifndef XTRACER_FBLOCK_H_INCLUDED
#define XTRACER_FBLOCK_H_INCLUDED

#include <cstddef>

namespace xtracer {
    namespace render {

struct frame_block_t {
	size_t x, y, width, height;
	int priority;

	frame_block_t();
	frame_block_t(size_t x, size_t y, size_t width, size_t height);

};

    } /* namespace render */
} /* namespace xtracer */

#endif /* XTRACER_FBLOCK_H_INCLUDED */
