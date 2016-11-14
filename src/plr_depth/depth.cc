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

#include "depth.h"

#define TILESIZE 32

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
        || !m_context->framebuffer
        || !m_context->scene
        || !m_context->scene->get_camera()
    ) return;

    render_depth();
}

void DRenderer::render_depth()
{
	// precalculate some constants
	const unsigned int w  = m_context->framebuffer->width();
	const unsigned int h  = m_context->framebuffer->height();
    const unsigned int t  = m_context->params.threads;
    const unsigned int aa = m_context->params.ssaa;

	const unsigned int dxt       = (w % TILESIZE > 0 ? 1 : 0);
	const unsigned int dyt       = (h % TILESIZE > 0 ? 1 : 0);
    const unsigned int numxtiles = w / TILESIZE + dxt;
	const unsigned int numytiles = h / TILESIZE + dyt;
	const unsigned int numtiles  = numxtiles * numytiles;

    float progress = 0;
	if (t) omp_set_num_threads(t);

    float one_over_h = 1.f / numtiles;
    float spp = (float)(aa * aa);
    double subpixel_size  = 1.f / (float)(aa);
    double subpixel_size2 = subpixel_size / 2.f;

    float sample_scaling = 1./ spp;

	#pragma omp parallel for schedule(dynamic, 1)
    for (size_t tile = 0; tile < numtiles; ++tile) {
        // tile offset
        const int ia = TILESIZE*(tile % numxtiles);
        const int ja = TILESIZE*(tile / numxtiles);

        for (int j = 0; j < TILESIZE; ++j) {
            float y = ja + j;
            if (y > h) continue;

            for (int i = 0; i < TILESIZE; ++i) {
                float x = ia + i;
                if (x > w) continue;

                ColorRGBAf color;

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
                        color += nimg::ColorRGBAf(depth, depth, depth, 1) * sample_scaling;
                    }
                }

                m_context->framebuffer->pixel(x, y) = color;
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
