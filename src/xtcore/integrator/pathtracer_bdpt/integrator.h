#ifndef XTCORE_INTEGRATOR_PATHTRACER_BDPT_H_INCLUDED
#define XTCORE_INTEGRATOR_PATHTRACER_BDPT_H_INCLUDED

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
        namespace pathtracer_bdpt {

struct PathVertex
{
    nmath::Vector3f          point;
    nmath::Vector3f          normal;
    nmath::Vector3f          texcoord;
    nimg::ColorRGBf          throughput;   // accumulated beta from path start to this vertex
    nmath::scalar_t          pdf_fwd;      // solid-angle PDF of sampling this vertex from the previous one
    nmath::scalar_t          pdf_rev;      // solid-angle PDF of sampling previous vertex from this one (for MIS)
    const xtcore::asset::IMaterial *material;
    HASH_ID                  obj_id;
    nmath::Vector3f          wo;           // outgoing direction at this vertex (toward previous vertex / camera)
    bool                     is_delta;     // was sampled via a delta BSDF / is a point light
    bool                     on_light;     // true for light-subpath origin vertices
};

class Integrator : public xtcore::render::IIntegrator
{
    public:
    struct area_light_t {
        HASH_ID object_id;
        const xtcore::asset::ISurface *surface;
        const xtcore::asset::IMaterial *material;
        nmath::scalar_t area;
        nmath::scalar_t select_weight;
    };

    virtual void setup_auxiliary();
    virtual void clean_auxiliary();
    virtual xtcore::render::integrator_metadata_t metadata() const {
        xtcore::render::integrator_metadata_t meta;
        meta.id = "pathtracer_bdpt";
        meta.name = "Pathtracer (BDPT)";
        meta.status = xtcore::render::INTEGRATOR_STATUS_EXPERIMENTAL;
        meta.description = "Bidirectional path tracer — connects camera and light subpaths with MIS weighting.";
        return meta;
    }
    virtual void render_tile(xtcore::render::tile_t *tile);

    private:
    std::vector<area_light_t> m_lights;
    std::vector<nmath::scalar_t> m_light_cdf;
    std::map<HASH_ID, size_t> m_light_index_by_objid;
    std::map<HASH_ID, nmath::scalar_t> m_light_select_pdf;
    nmath::scalar_t m_light_weight_sum;

    int trace_camera_path(const xtcore::Ray &primary_ray, size_t max_depth,
                          std::vector<PathVertex> &path,
                          nimg::ColorRGBf &env_contribution);
    int trace_light_path(size_t max_depth, std::vector<PathVertex> &path);
    nimg::ColorRGBf connect(const std::vector<PathVertex> &cam_path, int s,
                            const std::vector<PathVertex> &lgt_path, int t);
};

        } /* namespace pathtracer_bdpt */
    } /* namespace integrator */
} /* namespace xtcore */

#endif /* XTCORE_INTEGRATOR_PATHTRACER_BDPT_H_INCLUDED */
