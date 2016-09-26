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
	const unsigned int h = m_context->framebuffer->height();
	const unsigned int thread_count = m_context->params.threads;
	if (thread_count) {
		omp_set_num_threads(thread_count);
	}

	#pragma omp parallel for schedule(dynamic, 1)
	for (int y = 0; y < (int)h; y++) {
		for (int x = 0; x < (int)w; x++) {
			IntInfo info;
			memset(&info, 0, sizeof(info));
	 		Ray ray = m_context->scene->get_camera()->get_primary_ray((float)x, (float)y, (float)w, (float)h);
			std::string obj;
			NMath::scalar_t depth = (m_context->scene->intersection(ray, info, obj))
			? 1./log((m_context->scene->get_camera()->position - info.point).length())
			: INFINITY;
			m_context->framebuffer->pixel(x, y) = nimg::ColorRGBAf(depth, depth, depth, 1);
		}

	}
}
