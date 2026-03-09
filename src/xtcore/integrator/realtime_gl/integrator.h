#ifndef XTCORE_INTEGRATOR_REALTIME_GL_H_INCLUDED
#define XTCORE_INTEGRATOR_REALTIME_GL_H_INCLUDED

#include <xtcore/tile.h>
#include <xtcore/integrator.h>

namespace xtcore {
    namespace integrator {
        namespace realtime_gl {

class Integrator : public xtcore::render::IIntegrator
{
    public:
    virtual void render_tile(xtcore::render::tile_t *tile);
};

        } /* namespace realtime_gl */
    } /* namespace integrator */
} /* namespace xtcore */

#endif /* XTCORE_INTEGRATOR_REALTIME_GL_H_INCLUDED */
