#ifndef XTCORE_RENDERER_H_INCLUDED
#define XTCORE_RENDERER_H_INCLUDED

#include <nimg/pixmap.h>
#include "context.h"

namespace xtcore {
	namespace render {

class IRenderer
{
	public:
	virtual ~IRenderer();

	virtual void setup(context_t &context) = 0;
	virtual void render()                  = 0;

};

	} /* namespace render */
} /* namespace xtcore  */

#endif /* XTCORE_RENDERER_H_INCLUDED */
