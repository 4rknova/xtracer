#ifndef XTCORE_INTEGRATOR_PHOTON_MAPPING_H_INCLUDED
#define XTCORE_INTEGRATOR_PHOTON_MAPPING_H_INCLUDED

#include <vector>
#include <nmath/precision.h>
#include <nmath/vector.h>
#include <nimg/color.h>
#include <nimg/pixmap.h>
#include <xtcore/math/hitrecord.h>
#include <xtcore/math/kdtree.h>
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

    virtual xtcore::render::integrator_metadata_t metadata() const {
        xtcore::render::integrator_metadata_t meta;
        meta.id = "photon_mapping";
        meta.name = "Photon Mapping";
        meta.status = xtcore::render::INTEGRATOR_STATUS_EXPERIMENTAL;
        meta.description = "Photon mapping integrator for caustic and density-estimate experiments.";
        return meta;
    }
    virtual void configure(const std::map<std::string, std::string> &options);
    virtual void setup_auxiliary();
    virtual void clean_auxiliary();
    virtual void render_tile(xtcore::render::tile_t *tile);

    nimg::ColorRGBf eval(size_t depth, hit_result_t &in);
    const std::vector<nmath::Vector3f> &debug_global_points() const;
    const std::vector<nmath::Vector3f> &debug_caustic_points() const;

    struct Photon {
        nmath::Vector3f position;
        nmath::Vector3f incident;
        nmath::Vector3f normal;
        nimg::ColorRGBf power;
    };

    struct PhotonPositionAccessor {
        const nmath::Vector3f &operator()(const Photon &photon) const {
            return photon.position;
        }
    };

    struct AreaLight {
        HASH_ID object_id;
        const xtcore::asset::ISurface *surface;
        const xtcore::asset::IMaterial *material;
        nmath::scalar_t area;
        nmath::scalar_t weight_cdf;
    };

    struct CausticGuide {
        const xtcore::asset::ISurface *surface;
        nmath::scalar_t weight_cdf;
    };

    private:
    std::vector<Photon> m_global_photons;
    std::vector<Photon> m_caustic_photons;
    std::vector<nmath::Vector3f> m_debug_global_points;
    std::vector<nmath::Vector3f> m_debug_caustic_points;
    xtcore::math::KDTree3<Photon, PhotonPositionAccessor> m_global_map;
    xtcore::math::KDTree3<Photon, PhotonPositionAccessor> m_caustic_map;
    std::vector<AreaLight> m_lights;
    std::vector<CausticGuide> m_caustic_guides;
    nmath::scalar_t m_scene_diag;
    nmath::scalar_t m_gather_radius;
    nmath::scalar_t m_caustic_gather_radius;
    size_t m_gather_k;
    size_t m_caustic_gather_k;
    size_t m_emit_photons;
    size_t m_caustic_emit_photons;
    bool m_override_gather_radius;
    bool m_override_caustic_gather_radius;
    bool m_override_gather_k;
    bool m_override_caustic_gather_k;
    bool m_override_emit_photons;
    bool m_override_caustic_emit_photons;
    nmath::scalar_t m_config_gather_radius;
    nmath::scalar_t m_config_caustic_gather_radius;
    size_t m_config_gather_k;
    size_t m_config_caustic_gather_k;
    size_t m_config_emit_photons;
    size_t m_config_caustic_emit_photons;

    void build_photon_map();
    void trace_photon(const xtcore::Ray &ray,
                      const nimg::ColorRGBf &power,
                      nmath::scalar_t ior,
                      size_t depth,
                      bool has_specular_bounce,
                      bool collect_global,
                      bool collect_caustic);
    nimg::ColorRGBf estimate_indirect(const xtcore::math::KDTree3<Photon, PhotonPositionAccessor> &map,
                                      nmath::scalar_t gather_radius,
                                      size_t gather_k,
                                      const xtcore::hit_record_t &hit,
                                      const nimg::ColorRGBf &kd) const;
};

        } /* namespace photon_mapping */
    } /* namespace integrator */
} /* namespace xtcore */

#endif /* XTCORE_INTEGRATOR_PHOTON_MAPPING_H_INCLUDED */
