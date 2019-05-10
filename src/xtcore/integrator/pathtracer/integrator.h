#ifndef XTCORE_INTEGRATOR_PATHTRACER_H_INCLUDED
#define XTCORE_INTEGRATOR_PATHTRACER_H_INCLUDED

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
        namespace pathtracer {

class Integrator : public xtcore::render::IIntegrator
{
	public:
	virtual void render_tile(xtcore::render::tile_t *tile);
    nimg::ColorRGBf eval(size_t depth, hit_result_t &in);
};

        } /* namespace pathtracer */
    } /* namespace integrator */
} /* namespace xtcore */

#endif /* XTCORE_INTEGRATOR_PATHTRACER_H_INCLUDED */
