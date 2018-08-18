#ifndef XTCORE_INTEGRATOR_EMISSION_H_INCLUDED
#define XTCORE_INTEGRATOR_EMISSION_H_INCLUDED

#include <nmath/precision.h>
#include <nmath/vector.h>
#include <nimg/color.h>
#include <nimg/pixmap.h>
#include <nplatform/timer.h>
#include <xtcore/scene.h>
#include <xtcore/integrator.h>
#include "math/ray.h"
#include "math/hitrecord.h"

using nimg::ColorRGBf;
using nimg::Pixmap;

namespace xtcore {
    namespace integrator {
        namespace emission {

class Integrator : public xtcore::render::IIntegrator
{
	public:
	Integrator();

	virtual void setup(xtcore::render::context_t &context);
	virtual void render();

	private:
    xtcore::render::context_t *m_context;
};

        } /* namespace emission */
    } /* namespace integrator */
} /* namespace xtcore */

#endif /* XTCORE_INTEGRATOR_EMISSION_H_INCLUDED */
