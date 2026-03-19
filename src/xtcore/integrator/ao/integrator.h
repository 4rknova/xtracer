#ifndef XTCORE_INTEGRATOR_AO_H_INCLUDED
#define XTCORE_INTEGRATOR_AO_H_INCLUDED

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
        namespace ao {

class Integrator : public xtcore::render::IIntegrator
{
	public:
    Integrator();
    virtual void configure(const std::map<std::string, std::string> &options);
    virtual void render_tile(xtcore::render::tile_t *tile);

    private:
    nmath::scalar_t m_max_distance;
};

        } /* namespace ao */
    } /* namespace integrator */
} /* namespace xtcore */

#endif /* XTCORE_INTEGRATOR_AO_H_INCLUDED */
