#include <iostream>
#include <vector>
#include <iomanip>
#include <omp.h>

#include <nmath/mutil.h>
#include <nmath/prng.h>
#include <nmath/plane.h>
#include <nmath/sample.h>
#include <nimg/luminance.h>
#include <ncf/util.h>
#include <xtcore/log.h>

#include "renderer.h"

#define XTRACER_SETUP_DEFAULT_GI                false   /* Default gi flag value. */
#define XTRACER_SETUP_DEFAULT_GIVIZ             false   /* Default giviz flag value. */
#define XTRACER_SETUP_DEFAULT_PHOTON_COUNT      1000000 /* Default photon count for gi. */
#define XTRACER_SETUP_DEFAULT_PHOTON_SAMPLES    1000    /* Default photon samples. */
#define XTRACER_SETUP_DEFAULT_PHOTON_SRADIUS    25.0    /* Default photon sampling radius. */
#define XTRACER_SETUP_DEFAULT_PHOTON_POWERSC    1.25    /* Default photon power scaling factor. */
#define XTRACER_SETUP_DEFAULT_MAX_RDEPTH        4       /* Default maximum recursion depth. */

#define TILESIZE 32

using Util::String::path_comp;

Renderer::Renderer()
	: m_context(NULL)
{}

void Renderer::setup(xtracer::render::context_t &context)
{
    m_context = &context;
}

void Renderer::render()
{
	if (   !m_context
        || !m_context->framebuffer
        || !m_context->scene
        || !m_context->scene->get_camera()
    ) return;

    pass_ptrace();
	pass_rtrace();
}

void Renderer::pass_ptrace()
{
	if (!XTRACER_SETUP_DEFAULT_GI) return;

	unsigned int photon_count = XTRACER_SETUP_DEFAULT_PHOTON_COUNT; // Environment::handle().photon_count();

	Log::handle().log_message("Initiating the photon maps..", photon_count);
	Log::handle().log_message("Using %i photons..", photon_count);
	m_pm_global.init(photon_count);
	m_pm_caustic.init(photon_count);

	Log::handle().log_message("Distributing photons to light sources..", photon_count);
	// Calculate each light's contribution by using the intensity luminance.
	// In future revisions I should also take the light source's size into consideration.
    std::vector<light_t> lights;
    m_context->scene->get_light_sources(lights);

	std::vector<unsigned int> light_photons;
	{
		unsigned int light_count = lights.size();
		std::vector<scalar_t> light_contribution;			// Vector with each light's contribution.
		std::vector<scalar_t> light_luminance;				// Vector with each light's luminance.
		scalar_t light_total_luminance = 0;					// Total light luminance.

		// Calculate the luminance for each light and the total.
		std::vector<light_t>::iterator it = lights.begin();
		std::vector<light_t>::iterator et = lights.end();
		for (; it != et; ++it) {
			NMath::scalar_t lum = nimg::eval::luminance((*it).material->emissive);
			light_luminance.push_back(lum);
			light_total_luminance += lum;
		}

		// Calculate the contributions and number of photons per light.
		for (unsigned int i = 0; i < light_count; ++i) {
			light_contribution.push_back(light_luminance[i] / light_total_luminance);
			light_photons.push_back((scalar_t) (photon_count+1) * light_contribution[i]);
			Log::handle().log_message("- Light %i: %i photons, Luminance: %f, %f%% contribution", i,
				light_photons[i], light_luminance[i], light_contribution[i] * 100);
		}
	}

	Log::handle().log_message("Casting photons..");
	{
		// Photon tracing.
		unsigned int light_index = 0;
		std::vector<light_t>::iterator it = lights.begin();
		std::vector<light_t>::iterator et = lights.end();
		for (; it != et; ++it) {
			while (light_photons[light_index] > 0) {
				Ray ray = (*it).light->ray_sample();
				trace_photon(ray, 0, (*it).material->emissive
						* XTRACER_SETUP_DEFAULT_PHOTON_POWERSC
						, light_photons[light_index]);
			}

			light_index++;
		}
	}

	Log::handle().log_message("Balancing the photon map..");
	m_pm_global.balance();
}

bool Renderer::trace_photon(const Ray &ray, const unsigned int depth, const ColorRGBf power, unsigned int &map_capacity)
{
	if (depth > XTRACER_SETUP_DEFAULT_MAX_RDEPTH) return false;

	// Intersect.
	IntInfo info;
	memset(&info, 0, sizeof(info));
	std::string obj;

	if (!m_context->scene->intersection(ray, info, obj)) return false;

	// Get the material & texture.
    std::string &mat_id = m_context->scene->m_objects[obj]->material;
    std::string &tex_id = m_context->scene->m_objects[obj]->texture;
	Material *mat = m_context->scene->m_materials[mat_id];
	std::map<std::string, Texture2D *>::iterator it_tex = m_context->scene->m_textures.find(tex_id);

	// Check if there are photons left to consume.
	if (depth > 0 &&  map_capacity > 0) {
		// Store the photon.
		float pos[3]; // Photon position.
		float pwr[3]; // Photon intensity.
		float dir[3]; // Photon direction.

		pwr[0] = power.r();
		pwr[1] = power.g();
		pwr[2] = power.b();
		pos[0] = ray.origin.x;
		pos[1] = ray.origin.y;
		pos[2] = ray.origin.z;
		dir[0] = -ray.direction.x;
		dir[1] = -ray.direction.y;
		dir[2] = -ray.direction.z;

		m_pm_global.store(pos, pwr, dir);
		map_capacity--;
		std::cout << "\rCasting photons.. " << std::setw(12) << map_capacity << std::flush;
	}

	// Russian rulette.
	scalar_t avg_diff = (mat->diffuse.r() + mat->diffuse.g() + mat->diffuse.b()) / 3;
	scalar_t avg_spec = (mat->specular.r() + mat->specular.g() + mat->specular.b()) / 3;
	scalar_t avg_range = NMath::clamp_max(avg_diff + avg_spec, 1);
	scalar_t event = NMath::prng_c(0.0, avg_range);

	Ray nray;
	nray.origin = info.point;

	if (event < avg_diff) { // Interdiffuse.
		nray.direction = NMath::Sample::hemisphere(info.normal, -ray.direction);
		nray.origin += nray.direction * 0.5;

		ColorRGBf texcol = ColorRGBf(1, 1, 1);
		if (it_tex != m_context->scene->m_textures.end())
			texcol = ColorRGBf((*it_tex).second->sample((float)info.texcoord.x, (float)info.texcoord.y));

		trace_photon(nray, depth + 1, power * texcol * mat->diffuse, map_capacity);

		return true;
	}

	return false;
}

void Renderer::pass_rtrace()
{
	// precalculate some constants
	const unsigned int w       = m_context->framebuffer->width();
	const unsigned int h       = m_context->framebuffer->height();
	const unsigned int t       = m_context->params.threads;
	const unsigned int aa      = m_context->params.ssaa;
	const unsigned int samples = m_context->params.samples;

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
	double subpixel_size2 = subpixel_size / 2.0f;

	float sample_scaling = 1.0f / (samples * spp);

	#pragma omp parallel for schedule(dynamic, 1)
    for (unsigned int tile = 0; tile < numtiles; ++tile) {
		// tile offset
        const int ia = TILESIZE*(tile % numxtiles);
        const int ja = TILESIZE*(tile / numxtiles);

		// for every pixel in this tile, compute color
	    for (int j = 0; j < TILESIZE; ++j) {
			float y = ja + j;
			if (y > h) continue;

	        for( int i = 0; i < TILESIZE; ++i) {
				float x = ia + i;
				if (x > w) continue;

				ColorRGBf color;
				// antialiasing loop
				for (unsigned int fy = 0; fy < aa; ++fy) {
					for (unsigned int fx = 0; fx < aa; ++fx) {

						float rx = (float)x + (float)fx * subpixel_size + subpixel_size2;
						float ry = (float)y + (float)fy * subpixel_size + subpixel_size2;

                        for (float dofs = 0; dofs < samples; ++dofs) {
                            Ray primary_ray = m_context->scene->get_camera()->get_primary_ray(rx, ry, (float)w, (float)h);
                            color += trace_ray(primary_ray, primary_ray, XTRACER_SETUP_DEFAULT_MAX_RDEPTH + 1) * sample_scaling;
						}
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

ColorRGBf Renderer::trace_ray(const Ray &pray, const Ray &ray, const unsigned int depth,
	const scalar_t ior_src, const scalar_t ior_dst)
{
	IntInfo info;
	memset(&info, 0, sizeof(info));

	ColorRGBf gi_res(0, 0, 0);

	// Check for ray intersection
	std::string obj; // this will hold the object name
	if (m_context->scene->intersection(ray, info, obj)) {
		// get a pointer to the material
		if (!obj.empty()) {

			if (XTRACER_SETUP_DEFAULT_GI) { // Environment::handle().flag_gi()) {
				float irad[3] = {0,0,0};
				float posi[3] = {(float)info.point.x, (float)info.point.y, (float)info.point.z};

				float norm[3];
				norm[0] = (float)info.normal.x;
				norm[1] = (float)info.normal.y;
				norm[2] = (float)info.normal.z;

				m_pm_global.irradiance_estimate(irad, posi, norm,
					XTRACER_SETUP_DEFAULT_PHOTON_SRADIUS,
					XTRACER_SETUP_DEFAULT_PHOTON_SAMPLES
				);

				gi_res = ColorRGBf(irad[0], irad[1], irad[2]);

				if (XTRACER_SETUP_DEFAULT_GIVIZ)
					return gi_res;
			}

            std::string &mat_id = m_context->scene->m_objects[obj]->material;
            Material *mat = m_context->scene->m_materials[mat_id];

			// if the ray starts inside the geometry
			scalar_t dot_normal_dir = dot(info.normal, ray.direction);
			if (mat->transparency > EPSILON && dot_normal_dir > 0) info.normal = -info.normal;
			scalar_t ior_a = dot_normal_dir > 0 ? mat->ior : ior_src;
			scalar_t ior_b = dot_normal_dir > 0 ? ior_src  : mat->ior;

			return gi_res + shade(pray, ray, depth, info, obj, ior_a, ior_b);
		}
	}

	return ColorRGBf(0, 0, 0);
}

ColorRGBf Renderer::shade(const Ray &pray, const Ray &ray, const unsigned int depth,
	IntInfo &info, std::string &obj,
	const scalar_t ior_src, const scalar_t ior_dst)
{
	ColorRGBf color(0, 0, 0);

	// check if the depth limit was reached
	if (!depth)	return color;

	Object *mobj = m_context->scene->m_objects[obj];
	std::map<std::string, Texture2D *>::iterator it_tex = m_context->scene->m_textures.find(mobj->texture);
	std::map<std::string, Material  *>::iterator it_mat = m_context->scene->m_materials.find(mobj->material);

	if (it_mat == m_context->scene->m_materials.end()) return color;

	// shadows
	NMath::Vector3f n = info.normal;
	NMath::Vector3f p = info.point;

    std::string &mat_id = mobj->material;
	Material *mat = m_context->scene->m_materials[mat_id];

	// ambient
	color = mat->ambient * m_context->scene->ambient();

	scalar_t shadow_sample_scaling = 1.0f / m_context->params.samples;

    std::vector<light_t> lights;
    m_context->scene->get_light_sources(lights);
	std::vector<light_t>::iterator it = lights.begin();
	std::vector<light_t>::iterator et = lights.end();

	for (; it != et; ++it) {

		unsigned int tlshsamples = m_context->params.samples;
		scalar_t tlshscaling = shadow_sample_scaling;

		for (unsigned int shsamples = 0; shsamples < tlshsamples; ++shsamples) {
            NMath::Vector3f light_pos = (*it).light->point_sample();
			NMath::Vector3f v = light_pos - p;

			// Texture.
			ColorRGBf texcolor = ColorRGBf(1,1,1);
			if (it_tex != m_context->scene->m_textures.end()) {
				texcolor = ColorRGBf((*it_tex).second->sample((float)info.texcoord.x, (float)info.texcoord.y));
			}

			Ray sray;
			sray.direction = v.normalized();
			sray.origin = p;
			scalar_t distance = v.length();

			// if the point is not in shadow for this light
			std::string obj;
			IntInfo res;
			bool test = m_context->scene->intersection(sray, res, obj);

            std::map<std::string, Geometry*>::iterator git = m_context->scene->m_geometry.find(obj);
            std::map<std::string, Geometry*>::iterator get = m_context->scene->m_geometry.end();

            bool hits_light_geometry = (git != get && (*it).light == (*git).second);


			if (!test || res.t < EPSILON || res.t > distance || hits_light_geometry) {
                color += mat->shade(pray.origin, light_pos, (*it).material->emissive, texcolor, info) * tlshscaling;
			}
		}
	}

	// Fresnel
	float fr = 1.0;

	if ((mat->type == MATERIAL_PHONG) || (mat->type == MATERIAL_BLINNPHONG)) {
		// refraction
		if(mat->transparency > 0.0) {
			Ray refrray;
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
    			color *= mat->transparency;
			    color += ft * mat->transparency * trace_ray(pray, refrray, depth-1, ior_src, ior_dst) * mat->specular * mat->kspec;
            }
		}

		// reflection
		if(mat->reflectance > 0.0) {
			unsigned int tlmcsamples = m_context->params.samples;
			scalar_t tlmcscaling = 1.0f / (scalar_t)tlmcsamples;
			for (unsigned int mcsamples = 0; mcsamples < tlmcsamples; ++mcsamples) {
				Ray reflray;
				reflray.origin = p;
				reflray.direction = NMath::Sample::lobe(n, -ray.direction, mat->roughness == 0 ? mat->ksexp : mat->roughness);
				color += fr * (mat->reflectance * trace_ray(pray, reflray, depth-1) * mat->specular * mat->kspec * tlmcscaling);
			}
		}
	}

	return color;
}
