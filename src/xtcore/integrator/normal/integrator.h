#ifndef XTCORE_INTEGRATOR_NORMAL_H_INCLUDED
#define XTCORE_INTEGRATOR_NORMAL_H_INCLUDED

#include <nplatform/timer.h>
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
        namespace normal {

class Integrator : public xtcore::render::IIntegrator
{
	public:
	virtual void render_tile(xtcore::render::tile_t *tile);
};

        } /* namespace normal */
    } /* namespace integrator */
} /* namespace xtcore */

#endif /* XTCORE_INTEGRATOR_NORMAL_H_INCLUDED */
