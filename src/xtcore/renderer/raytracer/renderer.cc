#include <iostream>
#include <vector>
#include <iomanip>
#include <omp.h>

#include <nmath/mutil.h>
#include <nmath/prng.h>
#include <nmath/sample.h>
#include <nimg/luminance.h>
#include <ncf/util.h>
#include <xtcore/tile.h>
#include <xtcore/aa.h>
#include "renderer.h"

#include "profiler.h"

namespace xtcore {
    namespace renderer {
        namespace raytracer {

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
    xtcore::asset::ICamera   *cam = m_context->scene.get_camera(p->camera);

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
            nimg::ColorRGBf  color_sample;

            tile->read(aa_sample.pixel.x, aa_sample.pixel.y, color_pixel);

            for (float dofs = 0; dofs < p->samples; ++dofs) {
                xtcore::Ray primary_ray = cam->get_primary_ray(
                      aa_sample.coords.x, aa_sample.coords.y
                    , (float)(p->width)
                    , (float)(p->height)
                );
                color_sample += trace_ray(primary_ray, primary_ray, p->rdepth + 1) * (1./ p->samples);
		    }

            tile->write(aa_sample.pixel.x, aa_sample.pixel.y, nimg::ColorRGBf(color_pixel) + color_sample * aa_sample.weight);
        }
        tile->submit();
	}
}

ColorRGBf Renderer::trace_ray(const xtcore::Ray &pray, const xtcore::Ray &ray, const unsigned int depth,
	const scalar_t ior_src, const scalar_t ior_dst)
{
	xtcore::HitRecord info;
	memset(&info, 0, sizeof(info));

	ColorRGBf gi_res(0, 0, 0);

	// Check for ray intersection
	HASH_UINT64 obj;
	if (m_context->scene.intersection(ray, info, obj)) {
		// get a pointer to the material
        HASH_UINT64 mat_id = m_context->scene.m_objects[obj]->material;
        xtcore::asset::IMaterial *mat = m_context->scene.m_materials[mat_id];

		// if the ray starts inside the geometry
		scalar_t dot_normal_dir = dot(info.normal, ray.direction);

        NMath::scalar_t transparency = mat->get_scalar(MAT_SCALART_TRANSPARENCY);
        NMath::scalar_t ior          = mat->get_scalar(MAT_SCALART_IOR);

		if (transparency > EPSILON && dot_normal_dir > 0) info.normal = -info.normal;
		scalar_t ior_a = dot_normal_dir > 0 ? ior      : ior_src;
		scalar_t ior_b = dot_normal_dir > 0 ? ior_src  : ior;

		return gi_res + shade(pray, ray, depth, info, obj, ior_a, ior_b);
	}

    return m_context->scene.sample_cubemap(ray.direction);
}

ColorRGBf Renderer::shade(const xtcore::Ray &pray, const xtcore::Ray &ray, const unsigned int depth,
	xtcore::HitRecord &info, HASH_UINT64 &obj,
	const scalar_t ior_src, const scalar_t ior_dst)
{
	ColorRGBf color(0, 0, 0);

	// check if the depth limit was reached
	if (!depth)	return color;

	xtcore::asset::Object *mobj = m_context->scene.m_objects[obj];

	auto it_mat = m_context->scene.m_materials.find(mobj->material);

    if (it_mat == m_context->scene.m_materials.end()) return color;

	// shadows
	NMath::Vector3f n = info.normal;
	NMath::Vector3f p = info.point;

    HASH_UINT64 &mat_id = mobj->material;
	xtcore::asset::IMaterial *mat = m_context->scene.m_materials[mat_id];

    color = mat->get_sample(MAT_SAMPLER_EMISSIVE, info.texcoord)
          + mat->get_sample(MAT_SAMPLER_AMBIENT, info.texcoord) * m_context->scene.ambient();

    std::vector<xtcore::light_t> lights;
    m_context->scene.get_light_sources(lights);
	auto it = lights.begin();
	auto et = lights.end();

	for (; it != et; ++it) {
        NMath::Vector3f light_pos = (*it).light->point_sample();
		NMath::Vector3f v = light_pos - p;

		xtcore::Ray sray;
		sray.direction = v.normalized();
		sray.origin = p;
		scalar_t distance = v.length();

		// if the point is not in shadow for this light
		HASH_UINT64 obj;
		xtcore::HitRecord res;
		bool test = m_context->scene.intersection(sray, res, obj);

        auto oit = m_context->scene.m_objects.find(obj)
           , oet = m_context->scene.m_objects.end();

        if (oit == oet) continue;

        auto git = m_context->scene.m_geometry.find((*oit).second->geometry)
           , get = m_context->scene.m_geometry.end();

        bool hits_light_geometry = (git != get && (*it).light == (*git).second);

		if (!test || res.t < EPSILON || res.t > distance || hits_light_geometry) {
            ColorRGBf intensity;
            xtcore::asset::emitter_t e;
            e.position  = light_pos;
            e.intensity = (*it).material->get_sample(MAT_SAMPLER_EMISSIVE, NMath::Vector3f(0,0,0));
            mat->shade(intensity, m_context->scene.get_camera(m_context->params.camera), &e, info);
            color += intensity;
		}
	}

	// Fresnel
	float fr = 1.0;
    NMath::scalar_t reflectance  = mat->get_scalar(MAT_SCALART_REFLECTANCE);
    NMath::scalar_t transparency = mat->get_scalar(MAT_SCALART_TRANSPARENCY);
    NMath::scalar_t exponent     = mat->get_scalar(MAT_SCALART_EXPONENT);
    nimg::ColorRGBf specular     = mat->get_sample(MAT_SAMPLER_SPECULAR, info.texcoord);

    // refraction
	if (transparency > 0.f) {
		xtcore::Ray refrray;
		refrray.origin    = p;
		refrray.direction = (ray.direction).refracted(n, ior_src, ior_dst);

        if (dot(n, refrray.direction) > 0.) fr = 1.;
        else {
            float ci = 1. - dot( n, ray.direction);
            float ct = dot(-n, refrray.direction);
	    	float f1 = ior_src * ci;
   		    float f2 = ior_dst * ct;
            float f3 = ior_dst * ci;
            float f4 = ior_src * ct;
	    	float fra = pow((f1 - f2) / (f1 + f2), 2.0);
		    float frb = pow((f3 - f4) / (f3 + f4), 2.0);
            fr  = (fra + frb) * .5;

    		float ft = 1.0 - fr;
			color *= transparency;
		    color += ft * transparency * trace_ray(pray, refrray, depth-1, ior_src, ior_dst) * specular;
        }
	}

	if (reflectance > 0) {
		xtcore::Ray reflray;
		reflray.origin    = p;
		reflray.direction = NMath::Sample::lobe(n, -ray.direction, exponent);
		color += fr * (reflectance * trace_ray(pray, reflray, depth-1) * specular);
	}

	return color;
}

        } /* namespace raytracer */
    } /* namespace renderer */
} /* namespace xtcore */
