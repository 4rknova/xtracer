#include <iostream>
#include <vector>
#include <iomanip>
#include <omp.h>

#include <nmath/precision.h>
#include <nmath/mutil.h>
#include <nmath/prng.h>
#include <nmath/sample.h>

#include <nimg/luminance.h>

#include <ncf/util.h>

#include <xtcore/math/plane.h>
#include <xtcore/tile.h>
#include <xtcore/aa.h>
#include <xtcore/material.h>
#include <xtcore/proto.h>

#include "renderer.h"

namespace xtcore {
    namespace renderer {
        namespace emission {

using ncf::util::path_comp;

Renderer::Renderer()
	: m_context(NULL)
{}

void Renderer::setup(xtcore::render::context_t &context)
{
	m_context = &context;
}

void Renderer::render(void)
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

        bool hit = false;

        while (tile->samples.count() > 0) {
            xtcore::antialiasing::sample_t aa_sample;
            tile->samples.pop(aa_sample);
            nimg::ColorRGBAf color(0,0,0,0);
		    xtcore::HitRecord info;
            HASH_UINT64 obj;
            memset(&info, 0, sizeof(info));

            for (float dofs = 0; dofs < p->samples; ++dofs) {
             	xtcore::Ray ray = cam->get_primary_ray(
                      aa_sample.coords.x, aa_sample.coords.y
                    , (float)(p->width)
                    , (float)(p->height)
                );

                if (m_context->scene.intersection(ray, info, obj)) {
                    xtcore::asset::Object *o = m_context->scene.m_objects[obj];
                    xtcore::asset::IMaterial *m = m_context->scene.m_materials[o->material];
                    if (m->is_emissive()) {
                        nimg::ColorRGBAf c = m->get_sample(XTPROTO_LTRL_EMISSIVE, info.texcoord);
                        c.a(1);
                        tile->write(aa_sample.pixel.x, aa_sample.pixel.y, c);
                    }
                    hit = true;
                }
            }
        }
        tile->submit();
    }
}

        } /* namespace emission */
    } /* namespace renderer */
} /* namespace xtcore */
