#include <omp.h>
#include "integrator.h"

namespace xtcore {
	namespace render {

IIntegrator::IIntegrator()
    : ctx(0)
{}

IIntegrator::~IIntegrator()
{}

void IIntegrator::setup(context_t &context)
{
    ctx = &context;
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

    if (p->threads) omp_set_num_threads(p->threads);

    #pragma omp parallel for schedule(dynamic, 1)
    for (size_t i = 0; i < ctx->tiles.size(); ++i) {
        xtcore::render::tile_t *tile = &(ctx->tiles[i]);
        tile->init();

        xtcore::antialiasing::produce(tile, p->sample_distribution, p->aa, p->samples);

        render_tile(tile);

        tile->submit();
    }
}

	} /* namespace render */
} /* namespace xtcore */
