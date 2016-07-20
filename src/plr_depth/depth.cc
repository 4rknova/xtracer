#include <iostream>
#include <vector>
#include <iomanip>
#include <omp.h>

#include <nmath/precision.h>
#include <nmath/mutil.h>
#include <nmath/prng.h>
#include <nmath/plane.h>
#include <nmath/sample.h>
#include <nimg/luminance.hpp>
#include <ncf/util.h>
#include <xtcore/log.hpp>

#include "depth.h"

#define XTRACER_SETUP_DEFAULT_AA                3
using Util::String::path_comp;

DRenderer::DRenderer()
	: mFramebuffer(NULL)
	, mScene(NULL)
{}

void DRenderer::setup(Pixmap &fb, Scene &scene)
{
	mFramebuffer = &fb;
	mScene       = &scene;
}

void DRenderer::render(void)
{
	// precalculate some constants
	const unsigned int width  = mFramebuffer->width();
	const unsigned int height = mFramebuffer->height();

	Log::handle().log_message("Rendering..");

	// Explicitely set the thread count if requested.
	const unsigned int thread_count = 0;// Environment::handle().threads();
	if (thread_count) {
		omp_set_num_threads(thread_count);
	}

	#pragma omp parallel for schedule(dynamic, 1)
	for (int y = 0; y < (int)height; y++) {
		for (int x = 0; x < (int)width; x++) {
			IntInfo info;
			memset(&info, 0, sizeof(info));
	 		Ray ray = mScene->camera->get_primary_ray((float)x, (float)y, (float)width, (float)height);
			std::string obj;
			NMath::scalar_t depth = (mScene->intersection(ray, info, obj))
			? 1./log((mScene->camera->position - info.point).length())
			: INFINITY;
			mFramebuffer->pixel(x, y) = NImg::ColorRGBAf(depth, depth, depth, 1);
		}

	}
}
