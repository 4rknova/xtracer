#ifndef XTCORE_INTEGRATOR_H_INCLUDED
#define XTCORE_INTEGRATOR_H_INCLUDED

#include "tile.h"
#include "context.h"

namespace xtcore {
	namespace render {

class IIntegrator
{
	public:
    IIntegrator();
	virtual ~IIntegrator();

	void setup(context_t &context);
	void render();

    virtual void setup_auxiliary();
    virtual void clean_auxiliary();
    virtual void render_tile(tile_t *tile) = 0;

    xtcore::render::context_t *ctx;
};

	} /* namespace render */
} /* namespace xtcore  */

#include "integrator/stencil/integrator.h"
#include "integrator/depth/integrator.h"
#include "integrator/normal/integrator.h"
#include "integrator/uv/integrator.h"
#include "integrator/emission/integrator.h"
#include "integrator/pathtracer/integrator.h"
#include "integrator/pathtracer_is/integrator.h"
#include "integrator/photon_mapping/integrator.h"
#include "integrator/ao/integrator.h"
#if defined(XTRACER_ENABLE_REALTIME_GL) && XTRACER_ENABLE_REALTIME_GL
#include "integrator/realtime_gl/integrator.h"
#endif

#endif /* XTCORE_INTEGRATOR_H_INCLUDED */
