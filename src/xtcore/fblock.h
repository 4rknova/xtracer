#ifndef XTRACER_RBLOCK_H_INCLUDED
#define XTRACER_RBLOCK_H_INCLUDED

namespace xtracer {
	namespace render {

typedef struct rblock {
	unsigned int x;
	unsigned int y;
	unsigned int width;
	unsigned int height;

	int priority;

	rblock();
	rblock(unsigned int x,
		   unsigned int y,
		   unsigned int width,
		   unsigned int height);
} rblock_t;

	}
}

#endif /* XTRACER_RBLOCK_H_INCLUDED */
