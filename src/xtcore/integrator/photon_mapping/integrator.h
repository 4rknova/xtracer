#ifndef XTCORE_INTEGRATOR_PHOTON_MAPPING_H_INCLUDED
#define XTCORE_INTEGRATOR_PHOTON_MAPPING_H_INCLUDED

#include <vector>
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
        namespace photon_mapping {

class Integrator : public xtcore::render::IIntegrator
{
    public:
    Integrator();

    virtual void setup_auxiliary();
    virtual void clean_auxiliary();
    virtual void render_tile(xtcore::render::tile_t *tile);

    nimg::ColorRGBf eval(size_t depth, hit_result_t &in);

    struct Photon {
        nmath::Vector3f position;
        nmath::Vector3f incident;
        nmath::Vector3f normal;
        nimg::ColorRGBf power;
    };

    struct PhotonNode {
        Photon photon;
        int left;
        int right;
        int axis;
    };

    struct AreaLight {
        HASH_ID object_id;
        const xtcore::asset::ISurface *surface;
        const xtcore::asset::IMaterial *material;
        nmath::scalar_t area;
        nmath::scalar_t weight_cdf;
    };

    private:
    std::vector<Photon> m_photons;
    std::vector<PhotonNode> m_nodes;
    std::vector<AreaLight> m_lights;
    int m_root;
    nmath::scalar_t m_scene_diag;
    nmath::scalar_t m_gather_radius;
    size_t m_gather_k;
    size_t m_emit_photons;

    void build_photon_map();
    void trace_photon(const xtcore::Ray &ray, const nimg::ColorRGBf &power, nmath::scalar_t ior, size_t depth);
    int build_kdtree(size_t begin, size_t end, int axis);
    nimg::ColorRGBf estimate_indirect(const xtcore::hit_record_t &hit, const nimg::ColorRGBf &kd) const;
};

        } /* namespace photon_mapping */
    } /* namespace integrator */
} /* namespace xtcore */

#endif /* XTCORE_INTEGRATOR_PHOTON_MAPPING_H_INCLUDED */
