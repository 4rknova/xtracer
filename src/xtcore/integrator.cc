#ifdef _OPENMP
#include <omp.h>
#endif
#include "integrator.h"

namespace xtcore {
	namespace render {

IIntegrator::IIntegrator()
    : ctx(0)
    , abort_flag(0)
{}

IIntegrator::~IIntegrator()
{}

void IIntegrator::setup(context_t &context)
{
    ctx = &context;
}

void IIntegrator::configure(const std::map<std::string, std::string> &)
{
    // Do nothing by default.
}

void IIntegrator::set_abort_flag(const std::atomic<bool> *flag)
{
    abort_flag = flag;
}

bool IIntegrator::should_abort() const
{
    return abort_flag && abort_flag->load();
}

void IIntegrator::setup_auxiliary()
{
    // Do nothing
}

void IIntegrator::clean_auxiliary()
{
    // Do nothing
}

void IIntegrator::render()
{
    if (!ctx) return;

    xtcore::render::params_t *p = &(ctx->params);
    xtcore::asset::ICamera *cam = ctx->scene.get_camera(p->camera);

    if (!cam) return;

    setup_auxiliary();
    if (should_abort()) {
        clean_auxiliary();
        return;
    }

    size_t count = ctx->tiles.size();

    #ifdef _OPENMP
    if (p->threads) omp_set_num_threads(p->threads);
    #endif

    #pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < count; ++i) {
        if (should_abort()) continue;
        xtcore::render::tile_t *tile = &(ctx->tiles[i]);
        tile->init();
        if (should_abort()) continue;

        xtcore::antialiasing::produce(tile, p->sample_distribution, p->aa, p->samples);
        if (should_abort()) continue;

        render_tile(tile);
        if (should_abort()) continue;

        tile->submit();
    }

    clean_auxiliary();
}

	} /* namespace render */
} /* namespace xtcore */
