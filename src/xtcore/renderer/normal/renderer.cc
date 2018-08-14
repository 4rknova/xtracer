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

            nimg::ColorRGBAf color_pixel;
            tile->read(aa_sample.pixel.x, aa_sample.pixel.y, color_pixel);

            NMath::Vector3f  acc_normal = NMath::Vector3f(color_pixel.r(), color_pixel.g(), color_pixel.b());
            NMath::scalar_t  alpha  = color_pixel.a();

            NMath::Vector3f normal_sample;
            NMath::scalar_t alpha_sample = 0.f;
            for (float dofs = 0; dofs < p->samples; ++dofs) {
          	 	xtcore::Ray ray = cam->get_primary_ray(
                      aa_sample.coords.x, aa_sample.coords.y
                    , (float)(p->width)
                    , (float)(p->height)
                );

                if (m_context->scene.intersection(ray, info)) {
                    float weight = (1. / p->samples);
                    normal_sample += info.normal * weight;
                    alpha_sample  += weight;
                }
            }

            acc_normal += aa_sample.weight * (normal_sample * 0.5f + 0.5f);
            color_pixel = nimg::ColorRGBAf(acc_normal.x, acc_normal.y, acc_normal.z, 0.);
            color_pixel.a(alpha + aa_sample.weight * alpha_sample);
            tile->write(floor(aa_sample.pixel.x), floor(aa_sample.pixel.y), color_pixel);
        }

        // Normalize
        for (size_t x = tile->x0(); x < tile->x1(); ++x) {
            for (size_t y = tile->y0(); y < tile->y1(); ++y) {
                nimg::ColorRGBAf color;
                tile->read(x, y, color);
                NMath::Vector3f n(color.r(), color.g(), color.b());
                n = (n - 0.5f) * 2.0f;
                n.normalize();
                n = n * 0.5f + 0.5f;
                color = nimg::ColorRGBAf(n.x, n.y, n.z, color.a());
                tile->write(x, y, color);
            }
        }

        tile->submit();
    }
}

        } /* namespace normal */
    } /* namespace renderer */
} /* namespace xtcore */
