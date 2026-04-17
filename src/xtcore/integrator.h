#ifndef XTCORE_INTEGRATOR_H_INCLUDED
#define XTCORE_INTEGRATOR_H_INCLUDED

#include <map>
#include <string>
#include <atomic>

#include "tile.h"
#include "context.h"

namespace xtcore {
	namespace render {

enum integrator_status_t
{
    INTEGRATOR_STATUS_RECOMMENDED = 0,
    INTEGRATOR_STATUS_STABLE,
    INTEGRATOR_STATUS_EXPERIMENTAL,
    INTEGRATOR_STATUS_LEGACY,
    INTEGRATOR_STATUS_HIDDEN
};

struct integrator_metadata_t
{
    std::string id;
    std::string name;
    integrator_status_t status;
    std::string description;
    std::string replacement_id;

    integrator_metadata_t()
        : id()
        , name()
        , status(INTEGRATOR_STATUS_STABLE)
        , description()
        , replacement_id()
    {}
};

class IIntegrator
{
	public:
    IIntegrator();
	virtual ~IIntegrator();

	void setup(context_t &context);
    void render();
    virtual integrator_metadata_t metadata() const = 0;
    virtual void configure(const std::map<std::string, std::string> &options);
    void set_abort_flag(const std::atomic<bool> *flag);
    bool should_abort() const;

    virtual void setup_auxiliary();
    virtual void clean_auxiliary();
    virtual void render_tile(tile_t *tile) = 0;

    xtcore::render::context_t *ctx;
    const std::atomic<bool> *abort_flag;
};

	} /* namespace render */
} /* namespace xtcore  */

#include "integrator/debug_views/integrator.h"
#include "integrator/raytracer/integrator.h"
#include "integrator/pathtracer/integrator.h"
#include "integrator/pathtracer_mis/integrator.h"
#include "integrator/photon_mapping/integrator.h"
#include "integrator/ao/integrator.h"
#include "integrator/pathtracer_bdpt/integrator.h"

#endif /* XTCORE_INTEGRATOR_H_INCLUDED */
