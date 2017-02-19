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
#include <xtcore/tile.h>
#include <xtcore/aa.h>

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

int init()
{
   Log::handle().post_message("init");
   return 0;
}

int done()
{
  Log::handle().post_message("tile completed");
  return 0;
}


void DRenderer::render_depth()
{
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

        tile->setup(&init, &done);
        tile->init();

	    for (size_t y = tile->y0(); y < tile->y1(); ++y) {
	        for (size_t x = tile->x0(); x < tile->x1(); ++x) {

                nimg::ColorRGBf color;

                // antialiasing loop
                for (size_t aa; aa < samples.size(); ++aa) {
                    float rx = (float)x + samples[aa].x;
                    float ry = (float)y + samples[aa].y;

			        NMath::IntInfo info;
            		memset(&info, 0, sizeof(info));

                    for (float dofs = 0; dofs < s; ++dofs) {
                  	 	NMath::Ray ray = cam->get_primary_ray((float)rx, (float)ry, (float)w, (float)h);
                  		std::string obj;
                        float depth = 0.f;

                        if (m_context->scene->intersection(ray, info, obj)) {
                            NMath::scalar_t  d = (ray.origin - info.point).length();
                            depth = 1. / log(d);
                        }
                        color += nimg::ColorRGBAf(depth, depth, depth) * d;
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
