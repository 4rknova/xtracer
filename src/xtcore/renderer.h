#ifndef XTRACER_RENDERER_H_INCLUDED
#define XTRACER_RENDERER_H_INCLUDED

#include <nimg/pixmap.h>
#include "context.h"

namespace xtracer {
	namespace render {

class IRenderer
{
	public:
	virtual ~IRenderer();

	virtual void setup(context_t &context) = 0;
	virtual void render() = 0;

};

	} /* namespace render */
} /* namespace xtracer  */

#endif /* XTRACER_RENDERER_H_INCLUDED */
