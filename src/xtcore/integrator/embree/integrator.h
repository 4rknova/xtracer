#ifndef XTCORE_INTEGRATOR_EMBREE_H_INCLUDED
#define XTCORE_INTEGRATOR_EMBREE_H_INCLUDED

#include <xtcore/config.h>

#if FEATURE_IS_INCLUDED(FEATURE_EMBREE)

#include <embree3/rtcore.h>
#include <xtcore/integrator.h>

namespace xtcore {
    namespace integrator {
        namespace embree {

class Integrator : public xtcore::render::IIntegrator
{
    RTCDevice m_device;
    RTCScene  m_scene;

	public:
    void setup_auxiliary();
    void clean_auxiliary();
    void render_tile(xtcore::render::tile_t *tile);
};

        } /* namespace embree */
    } /* namespace integrator */
} /* namespace xtcore */

#endif /* FEATURE_EMBREE */

#endif /* XTCORE_INTEGRATOR_EMBREE_H_INCLUDED */
