#include "fblock.h"

namespace xtracer {
	namespace render {

frame_block_t::frame_block_t()
	: x        (0)
	, y        (0)
	, width    (0)
	, height   (0)
	, priority (0)
{}

frame_block_t::frame_block_t(
               size_t x,
		  	   size_t y,
	  	       size_t width,
  			   size_t height)
{
	this->x        = x;
	this->y        = y;
	this->width    = width;
	this->height   = height;
	this->priority = 0;
}

	} /* namespace render */
} /* namespace xtracer */
