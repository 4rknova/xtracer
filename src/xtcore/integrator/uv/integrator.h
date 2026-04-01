#ifndef XTCORE_INTEGRATOR_UV_H_INCLUDED
#define XTCORE_INTEGRATOR_UV_H_INCLUDED


#include <nmath/precision.h>
#include <nmath/vector.h>

#include <nimg/color.h>
#include <nimg/pixmap.h>

#include <xtcore/math/hitrecord.h>
#include <xtcore/math/ray.h>
#include <xtcore/scene.h>
#include <xtcore/integrator.h>

using nimg::ColorRGBf;
using nimg::Pixmap;

namespace xtcore {
    namespace integrator {
        namespace uv {

class Integrator : public xtcore::render::IIntegrator
{
	public:
    virtual xtcore::render::integrator_metadata_t metadata() const {
        xtcore::render::integrator_metadata_t meta;
        meta.id = "uv";
        meta.name = "UV";
        meta.status = xtcore::render::INTEGRATOR_STATUS_HIDDEN;
        meta.description = "UV debug integrator.";
        return meta;
    }
	virtual void render_tile(xtcore::render::tile_t *tile);
};

        } /* namespace uv */
    } /* namespace integrator */
} /* namespace xtcore */

#endif /* XTCORE_INTEGRATOR_UV_H_INCLUDED */
