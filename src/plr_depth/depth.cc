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
#include <xtcore/log.h>
#include <xtcore/fblock.h>

#include "depth.h"

using Util::String::path_comp;

DRenderer::DRenderer()
	: m_context(NULL)
{}

void DRenderer::setup(xtracer::render::context_t &context)
{
	m_context = &context;
}

void DRenderer::render(void)
{
    if (   !m_context
        || !m_context->scene
        || !m_context->scene->get_camera()
    ) return;

    render_depth();
}

xtracer::render::callback init()
{
   Log::handle().log_message("init");
   return 0;
}

void DRenderer::render_depth()
{
	// precalculate some constants
	const size_t w  = m_context->params.width;
	const size_t h  = m_context->params.height;
    const size_t t  = m_context->params.threads;
    const size_t aa = m_context->params.ssaa;

    float progress = 0;
	if (t) omp_set_num_threads(t);

    float spp = (float)(aa * aa);
    double subpixel_size  = 1.f / (float)(aa);
    double subpixel_size2 = subpixel_size / 2.f;

    float sample_scaling = 1./ spp;
    size_t tile_count = m_context->tiles.size();
	float one_over_h = 1.f / tile_count;

	#pragma omp parallel for schedule(dynamic, 1)
    for (size_t i = 0; i < tile_count; ++i) {
        xtracer::render::tile_t *tile = &(m_context->tiles[i]);

        tile->setup(0, 0);
        tile->init();

	    for (size_t y = tile->y0(); y <= tile->y1(); ++y) {
	        for (size_t x = tile->x0(); x <= tile->x1(); ++x) {

                nimg::ColorRGBf color;

                // antialiasing loop
                for (unsigned int fy = 0; fy < aa; ++fy) {
                    for (unsigned int fx = 0; fx < aa; ++fx) {

                        float rx = (float)x + (float)fx * subpixel_size + subpixel_size2;
                        float ry = (float)y + (float)fy * subpixel_size + subpixel_size2;

			            NMath::IntInfo info;
            			memset(&info, 0, sizeof(info));
            	 		NMath::Ray ray = m_context->scene->get_camera()->get_primary_ray((float)rx, (float)ry, (float)w, (float)h);
            			std::string obj;

                        float depth = 0.f;

                        if (m_context->scene->intersection(ray, info, obj)) {
                            NMath::scalar_t  d = (ray.origin - info.point).length();
                            depth = 1. / log(d);
                        }
                        color += nimg::ColorRGBAf(depth, depth, depth) * sample_scaling;
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
                      << progress * one_over_h * 100.f << "%"
                      << " @ " << omp_get_num_threads() << "T" << std::flush;
		}

    }
}
