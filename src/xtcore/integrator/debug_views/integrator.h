#ifndef XTCORE_INTEGRATOR_DEBUG_VIEWS_H_INCLUDED
#define XTCORE_INTEGRATOR_DEBUG_VIEWS_H_INCLUDED

#include <map>
#include <set>
#include <string>
#include <vector>

#include <xtcore/strpool.h>
#include <xtcore/tile.h>
#include <xtcore/integrator.h>

namespace xtcore {
    namespace integrator {
        namespace debug_views {

class Integrator : public xtcore::render::IIntegrator
{
    public:
    enum view_mode_t {
        VIEW_DEPTH = 0,
        VIEW_STENCIL,
        VIEW_NORMAL,
        VIEW_UV,
        VIEW_EMISSION,
        VIEW_OBJECT_MASK,
        VIEW_ENVIRONMENT
    };

    enum depth_encoding_t {
        DEPTH_ENCODING_LEGACY = 0,
        DEPTH_ENCODING_LINEAR,
        DEPTH_ENCODING_LOG,
        DEPTH_ENCODING_INVERSE
    };

    explicit Integrator(view_mode_t mode = VIEW_NORMAL);

    virtual xtcore::render::integrator_metadata_t metadata() const;
    virtual void configure(const std::map<std::string, std::string> &options);
    virtual void setup_auxiliary();
    virtual void render_tile(xtcore::render::tile_t *tile);

    private:
    view_mode_t m_mode;
    depth_encoding_t m_encoding;
    nmath::scalar_t m_max_distance;
    std::vector<std::string> m_mask_names;
    std::set<HASH_ID>        m_mask_ids;
    bool m_ignore_geometry = false;
};

        } /* namespace debug_views */
    } /* namespace integrator */
} /* namespace xtcore */

#endif /* XTCORE_INTEGRATOR_DEBUG_VIEWS_H_INCLUDED */
