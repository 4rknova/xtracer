#ifndef XTCORE_INTEGRATOR_DEPTH_H_INCLUDED
#define XTCORE_INTEGRATOR_DEPTH_H_INCLUDED

#include <nmath/precision.h>
#include <nmath/vector.h>
#include <nimg/color.h>
#include <nimg/pixmap.h>
#include <nplatform/timer.h>
#include <xtcore/scene.h>
#include <xtcore/integrator.h>
#include <xtcore/math/ray.h>
#include <xtcore/math/hitrecord.h>

using nimg::ColorRGBf;
using nimg::Pixmap;

namespace xtcore {
    namespace integrator {
        namespace depth {

class Integrator : public xtcore::render::IIntegrator
{
	public:
    void render_tile(xtcore::render::tile_t *tile);
};

        } /* namespace depth */
    } /* namespace integrator */
} /* namespace xtcore */

#endif /* XTCORE_INTEGRATOR_DEPTH_H_INCLUDED */
