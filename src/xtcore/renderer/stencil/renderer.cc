#include <iostream>
#include <vector>
#include <iomanip>
#include <omp.h>

#include <nmath/precision.h>
#include <nmath/mutil.h>
#include <nmath/prng.h>
#include <nmath/plane.h>
#include <nmath/sample.h>
#include <nimg/luminance.h>
#include <ncf/util.h>
#include <xtcore/tile.h>
#include <xtcore/aa.h>
#include "renderer.h"

namespace xtcore {
    namespace renderer {
        namespace stencil {

using ncf::util::path_comp;

Renderer::Renderer()
	: m_context(NULL)
{}

void Renderer::setup(xtcore::render::context_t &context)
{
	m_context = &context;
}

void Renderer::render(void)
{
    if (!m_context) return;

    xtcore::render::params_t *p   = &(m_context->params);
    xtcore::assets::ICamera  *cam = m_context->scene.get_camera(p->camera);

    if (!cam) return;

	if (p->threads) omp_set_num_threads(p->threads);

	#pragma omp parallel for schedule(dynamic, 1)
    for (size_t i = 0; i < m_context->tiles.size(); ++i) {
        xtcore::render::tile_t *tile = &(m_context->tiles[i]);
        tile->init();
        m_context->aa_sampler.produce(tile, p->aa);

        bool hit = false;

        while (tile->samples.count() > 0) {
            xtcore::antialiasing::sample_t aa_sample;
            tile->samples.pop(aa_sample);
            nimg::ColorRGBAf color(0,0,0,0);
		    NMath::IntInfo info;
            HASH_UINT64 obj;
            memset(&info, 0, sizeof(info));

            for (float dofs = 0; dofs < p->samples; ++dofs) {
             	NMath::Ray ray = cam->get_primary_ray(
                      aa_sample.coords.x, aa_sample.coords.y
                    , (float)(p->width)
                    , (float)(p->height)
                );

                if (m_context->scene.intersection(ray, info, obj)) {
                    tile->write(aa_sample.pixel.x, aa_sample.pixel.y, nimg::ColorRGBAf(1,1,1,1));
                    hit = true;
                }
            }
        }
        tile->submit();
    }
}

        } /* namespace stencil */
    } /* namespace renderer */
} /* namespace xtcore */
