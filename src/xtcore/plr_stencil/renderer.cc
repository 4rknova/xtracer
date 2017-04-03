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
#include "../tile.h"
#include "../aa.h"

#include "renderer.h"

namespace xtracer {
    namespace renderer {
        namespace stencil {

using ncf::util::path_comp;

Renderer::Renderer()
	: m_context(NULL)
{}

void Renderer::setup(xtracer::render::context_t &context)
{
	m_context = &context;
}

void Renderer::render(void)
{
    if (   !m_context
        || !m_context->scene
        || !m_context->scene->get_camera()
    ) return;

	// precalculate some constants
	const size_t w          = m_context->params.width;
	const size_t h          = m_context->params.height;
    const size_t t          = m_context->params.threads;
    const size_t s          = m_context->params.samples;
    const size_t tile_count = m_context->tiles.size();

    xtracer::antialiasing::SampleSet samples;
    xtracer::antialiasing::gen_samples_ssaa(samples, m_context->params.ssaa);
    xtracer::assets::ICamera *cam = m_context->scene->get_camera();
    float d = 1.f / (s * samples.size());

    float progress = 0;
	if (t) omp_set_num_threads(t);

	#pragma omp parallel for schedule(dynamic, 1)
    for (size_t i = 0; i < tile_count; ++i) {
        xtracer::render::tile_t *tile = &(m_context->tiles[i]);

        tile->init();

	    for (size_t y = tile->y0(); y < tile->y1(); ++y) {
	        for (size_t x = tile->x0(); x < tile->x1(); ++x) {

                ColorRGBf color;

                for (size_t aa = 0; aa < samples.size(); ++aa) {
                    float rx = (float)x + samples[aa].x;
                    float ry = (float)y + samples[aa].y;

			        NMath::IntInfo info;
                    std::string obj;
                    for (float dofs = 0; dofs < s; ++dofs) {
                  	 	NMath::Ray ray = cam->get_primary_ray((float)rx, (float)ry, (float)w, (float)h);
                        float res = m_context->scene->intersection(ray, info, obj) ? 1. : 0.;
                        color += nimg::ColorRGBAf(res, res, res) * d;
                    }
                }

                tile->write(x, y, color);
            }
        }

		#pragma omp critical
		{
			++progress;
			std::cout.setf(std::ios::fixed, std::ios::floatfield);
			std::cout.setf(std::ios::showpoint);
			std::cout << "\rRendering "
                      << std::setw(6) << std::setprecision(2)
                      << progress / tile_count * 100.f << "%"
                      << " @ " << omp_get_num_threads() << "T" << std::flush;
		}
    }
}

        } /* namespace stencil */
    } /* namespace renderer */
} /* namespace xtracer */
