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

#include "integrator.h"

namespace xtcore {
    namespace integrator {
        namespace raymarcher {

#define MAX_ITERATIONS (200)

Integrator::Integrator()
	: m_context(NULL)
{}

void Integrator::setup(xtcore::render::context_t &context)
{
	m_context = &context;
}

nimg::ColorRGBf Integrator::eval(size_t depth, const xtcore::Ray &ray)
{
    if (depth == 0) return nimg::ColorRGBf(0,0,0);

    NMath::Vector3f p = ray.origin;
    NMath::scalar_t d = INFINITY;   // Distance
    size_t iterations = MAX_ITERATIONS;
    bool done = false;
    bool hit  = false;

    HASH_UINT64 obj;

    while (!done) {
        NMath::scalar_t distance = m_context->scene.distance(p, obj);
        p += ray.direction * distance;
        hit = (distance < EPSILON);
        if (hit) d = distance;
        --iterations;
        done = ((iterations == 0) || hit);
    }

    if (hit) {
    /*
        // Calculate the normal: Gradient via Forward differencing
        NMath::Vector3f n;
        HASH_ID id;
        n.x = m_context->scene.distance(NMath::Vector3f(p.x + EPSILON, p.y, p.z), id);
        n.y = m_context->scene.distance(NMath::Vector3f(p.x, p.y + EPSILON, p.z), id);
        n.z = m_context->scene.distance(NMath::Vector3f(p.x, p.y, p.z + EPSILON), id);
    */
        float c = (MAX_ITERATIONS - iterations) / (float)MAX_ITERATIONS;
        return nimg::ColorRGBf(c, c, c);
    }

    return m_context->scene.sample_environment(ray.direction);
}

void Integrator::render()
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
            nimg::ColorRGBAf color_pixel;
            tile->read(aa_sample.pixel.x, aa_sample.pixel.y, color_pixel);

            for (size_t i = 0; i < p->samples; ++i) {
          	 	xtcore::Ray ray = cam->get_primary_ray(
                      aa_sample.coords.x, aa_sample.coords.y
                    , (float)(p->width)
                    , (float)(p->height)
                );

                color_pixel += eval(p->rdepth, ray) * aa_sample.weight * (1.f/p->samples);
            }

            color_pixel.a(1);
            tile->write(floor(aa_sample.pixel.x), floor(aa_sample.pixel.y), color_pixel);
        }

        tile->submit();
    }
}

        } /* namespace raymarcher */
    } /* namespace integrator */
} /* namespace xtcore */
