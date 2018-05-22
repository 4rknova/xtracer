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
        namespace pathtracer {

#define RADIUS (128.0)


Renderer::Renderer()
	: m_context(NULL)
{}

void Renderer::setup(xtcore::render::context_t &context)
{
	m_context = &context;
}

nimg::ColorRGBf Renderer::eval(size_t depth, const xtcore::Ray &ray, xtcore::HitRecord &info)
{
    if (depth == 0) return nimg::ColorRGBf(0,0,0);

    HASH_UINT64 obj;

    if (m_context->scene.intersection(ray, info, obj)) {
        xtcore::Ray r;
        r.origin    = info.point + info.normal;
        r.direction = NMath::Sample::hemisphere(info.normal, info.normal);

        xtcore::asset::Object    *o = m_context->scene.m_objects[obj];
        xtcore::asset::IMaterial *m = m_context->scene.m_materials[o->material];

        if (m->is_emissive()) {
            return m->get_sample("emissive", info.texcoord);
        }
        else
        {
            xtcore::HitRecord hit;
            return m->get_sample("diffuse", info.texcoord) * eval(--depth, r, hit);
        }
    }
    else
    {
        return m_context->scene.m_environment->sample(ray.direction);
    }
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

            nimg::ColorRGBAf color_pixel;
            tile->read(aa_sample.pixel.x, aa_sample.pixel.y, color_pixel);

            for (size_t i = 0; i < p->samples; ++i) {
          	 	xtcore::Ray ray = cam->get_primary_ray(
                      aa_sample.coords.x, aa_sample.coords.y
                    , (float)(p->width)
                    , (float)(p->height)
                );

                xtcore::HitRecord info;
                memset(&info, 0 ,sizeof(info));
                color_pixel += eval(p->rdepth, ray, info) * aa_sample.weight * (1.f/p->samples);
            }

            color_pixel.a(1);
            tile->write(floor(aa_sample.pixel.x), floor(aa_sample.pixel.y), color_pixel);
        }

        tile->submit();
    }
}

        } /* namespace pathtracer */
    } /* namespace renderer */
} /* namespace xtcore */
