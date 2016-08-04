#ifndef XTRACER_RENDERER_H_INCLUDED
#define XTRACER_RENDERER_H_INCLUDED

#include <nimg/pixmap.h>
#include "context.h"

namespace XT {
	namespace Render {

class IRenderer
{
	public:
	virtual ~IRenderer();

	virtual void setup(Context &context) = 0;
	virtual void render() = 0;

};

	}
}

#endif /* XTRACER_RENDERER_H_INCLUDED */
