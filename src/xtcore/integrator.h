#ifndef XTCORE_INTEGRATOR_H_INCLUDED
#define XTCORE_INTEGRATOR_H_INCLUDED

#include <nimg/pixmap.h>
#include "context.h"

namespace xtcore {
	namespace render {

class IIntegrator
{
	public:
	virtual ~IIntegrator();

	virtual void setup(context_t &context) = 0;
	virtual void render()                  = 0;

};

	} /* namespace render */
} /* namespace xtcore  */

#include "integrator/stencil/integrator.h"
#include "integrator/depth/integrator.h"
#include "integrator/normal/integrator.h"
#include "integrator/uv/integrator.h"
#include "integrator/emission/integrator.h"
#include "integrator/raytracer/integrator.h"
#include "integrator/pathtracer/integrator.h"
#include "integrator/raymarcher/integrator.h"

#endif /* XTCORE_INTEGRATOR_H_INCLUDED */
