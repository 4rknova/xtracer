#ifndef XTCORE_INTEGRATOR_PATHTRACER_IS_H_INCLUDED
#define XTCORE_INTEGRATOR_PATHTRACER_IS_H_INCLUDED

#include <map>
#include <vector>
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
        namespace pathtracer_is {

class Integrator : public xtcore::render::IIntegrator
{
    public:
    struct area_light_t {
        HASH_ID object_id;
        const xtcore::asset::ISurface *surface;
        const xtcore::asset::IMaterial *material;
        nmath::scalar_t area;
    };

    virtual void setup_auxiliary();
    virtual void clean_auxiliary();
    virtual xtcore::render::integrator_metadata_t metadata() const {
        xtcore::render::integrator_metadata_t meta;
        meta.id = "pathtracer_mis";
        meta.name = "Pathtracer (MIS Diffuse)";
        meta.status = xtcore::render::INTEGRATOR_STATUS_LEGACY;
        meta.description = "Diffuse-focused MIS path tracer kept as a simpler baseline.";
        meta.replacement_id = "pathtracer_mis_full";
        return meta;
    }
    virtual void render_tile(xtcore::render::tile_t *tile);
    nimg::ColorRGBf eval(size_t depth, hit_result_t &in);

    private:
    std::vector<area_light_t> m_lights;
    std::map<HASH_ID, size_t> m_light_index_by_objid;
};

        } /* namespace pathtracer_is */
    } /* namespace integrator */
} /* namespace xtcore */

#endif /* XTCORE_INTEGRATOR_PATHTRACER_IS_H_INCLUDED */
