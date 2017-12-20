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
        namespace normal {

Renderer::Renderer()
	: m_context(NULL)
{}

void Renderer::setup(xtcore::render::context_t &context)
{
	m_context = &context;
}

void Renderer::render()
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

        while (tile->samples.count() > 0) {
            xtcore::antialiasing::sample_t aa_sample;
            tile->samples.pop(aa_sample);
            NMath::Vector3f  normal;

            tile->read(aa_sample.pixel.x, aa_sample.pixel.y, color_pixel);

	        NMath::IntInfo info;
            HASH_UINT64 obj;
       		memset(&info, 0, sizeof(info));

            for (float dofs = 0; dofs < p->samples; ++dofs) {
          	 	NMath::Ray ray = cam->get_primary_ray(
                      aa_sample.coords.x, aa_sample.coords.y
                    , (float)(p->width)
                    , (float)(p->height)
                );
                NMath::Vector3f sample_normal;

                if (m_context->scene.intersection(ray, info, obj)) {
                    sample_normal = info.normal;
                }
                normal += sample_normal * (1. / p->samples);
            }
            normal.normalize();
            nimg::ColorRGBAf color_pixel(normal.x, normal.y, normal.z, 1.f);
            tile->write(floor(aa_sample.pixel.x), floor(aa_sample.pixel.y), color_pixel * aa_sample.weight);
        }
        tile->submit();
    }
}

        } /* namespace normal */
    } /* namespace renderer */
} /* namespace xtcore */
