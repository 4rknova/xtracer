#ifndef XTCORE_INTEGRATOR_DEPTH_H_INCLUDED
#define XTCORE_INTEGRATOR_DEPTH_H_INCLUDED

#include <xtcore/tile.h>
#include <xtcore/integrator.h>

using nimg::ColorRGBf;
using nimg::Pixmap;

namespace xtcore {
    namespace integrator {
        namespace depth {

class Integrator : public xtcore::render::IIntegrator
{
	public:
    Integrator();
    virtual void configure(const std::map<std::string, std::string> &options);
    void render_tile(xtcore::render::tile_t *tile);

    private:
    enum depth_encoding_t {
        DEPTH_ENCODING_LEGACY = 0,
        DEPTH_ENCODING_LINEAR,
        DEPTH_ENCODING_LOG,
        DEPTH_ENCODING_INVERSE
    };

    depth_encoding_t m_encoding;
    nmath::scalar_t m_max_distance;
};

        } /* namespace depth */
    } /* namespace integrator */
} /* namespace xtcore */

#endif /* XTCORE_INTEGRATOR_DEPTH_H_INCLUDED */
