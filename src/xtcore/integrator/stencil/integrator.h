#ifndef XTCORE_INTEGRATOR_STENCIL_H_INCLUDED
#define XTCORE_INTEGRATOR_STENCIL_H_INCLUDED

#include <xtcore/tile.h>
#include <xtcore/integrator.h>

namespace xtcore {
    namespace integrator {
        namespace stencil {

class Integrator : public xtcore::render::IIntegrator
{
	public:
    virtual xtcore::render::integrator_metadata_t metadata() const {
        xtcore::render::integrator_metadata_t meta;
        meta.id = "stencil";
        meta.name = "Stencil";
        meta.status = xtcore::render::INTEGRATOR_STATUS_HIDDEN;
        meta.description = "Stencil debug integrator.";
        return meta;
    }
	virtual void render_tile(xtcore::render::tile_t *tile);
};

        } /* namespace stencil */
    } /* namespace integrator */
} /* namespace xtcore */

#endif /* XTCORE_INTEGRATOR_STENCIL_H_INCLUDED */
