#include "fblock.h"

namespace xtracer {
	namespace render {

rblock::rblock()
	: x        (0)
	, y        (0)
	, width    (0)
	, height   (0)
	, priority (0)
{}

rblock::rblock(unsigned int x,
		  	   unsigned int y,
	  	       unsigned int width,
  			   unsigned int height)
{
	this->x        = x;
	this->y        = y;
	this->width    = width;
	this->height   = height;
	this->priority = 0;
}

	}
}
