#include <iostream>
#include <vector>
#include <iomanip>
#include <omp.h>

#include <nmath/precision.h>
#include <nmath/mutil.h>
#include <nmath/prng.h>
#include <nmath/sample.h>
#include <xtcore/math/plane.h>
#include <nimg/luminance.h>
#include <ncf/util.h>
#include <xtcore/tile.h>
#include <xtcore/aa.h>

#include "renderer.h"

namespace xtcore {
    namespace renderer {
        namespace uv {

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
    xtcore::asset::ICamera  *cam = m_context->scene.get_camera(p->camera);
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

	        xtcore::HitRecord info;
            HASH_UINT64 obj;
       		memset(&info, 0, sizeof(info));

            nimg::ColorRGBAf color_pixel;
            tile->read(aa_sample.pixel.x, aa_sample.pixel.y, color_pixel);

            NMath::Vector3f  acc_uv = NMath::Vector3f(color_pixel.r(), color_pixel.g(), color_pixel.b());
            NMath::scalar_t  alpha  = color_pixel.a();

            NMath::Vector3f uv_sample;
            NMath::scalar_t alpha_sample = 0.f;
            for (float dofs = 0; dofs < p->samples; ++dofs) {
          	 	xtcore::Ray ray = cam->get_primary_ray(
                      aa_sample.coords.x, aa_sample.coords.y
                    , (float)(p->width)
                    , (float)(p->height)
                );

                if (m_context->scene.intersection(ray, info, obj)) {
                    float weight = (1. / p->samples);
                    uv_sample += info.texcoord * weight;
                    alpha_sample  += weight;
                }
            }

            acc_uv += aa_sample.weight * (uv_sample * 0.5f + 0.5f);
            color_pixel = nimg::ColorRGBAf(acc_uv.x, acc_uv.y, acc_uv.z, 0.);
            color_pixel.a(alpha + aa_sample.weight * alpha_sample);
            tile->write(floor(aa_sample.pixel.x), floor(aa_sample.pixel.y), color_pixel);
        }

        tile->submit();
    }
}

        } /* namespace uv */
    } /* namespace renderer */
} /* namespace xtcore */
