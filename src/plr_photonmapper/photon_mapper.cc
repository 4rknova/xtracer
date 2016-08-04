#include <iostream>
#include <vector>
#include <iomanip>
#include <omp.h>

#include <nmath/mutil.h>
#include <nmath/prng.h>
#include <nmath/plane.h>
#include <nmath/sample.h>
#include <nimg/luminance.hpp>
#include <ncf/util.h>
//#include "object.hpp"
//#include "argparse.hpp"
#include <xtcore/log.hpp>
#include "photon_mapper.h"

#define XTRACER_SETUP_DEFAULT_AA                3
#define XTRACER_SETUP_DEFAULT_GI                false   /* Default gi flag value. */
#define XTRACER_SETUP_DEFAULT_GIVIZ             false   /* Default giviz flag value. */
#define XTRACER_SETUP_DEFAULT_PHOTON_COUNT      1000000 /* Default photon count for gi. */
#define XTRACER_SETUP_DEFAULT_PHOTON_SAMPLES    1000    /* Default photon samples. */
#define XTRACER_SETUP_DEFAULT_PHOTON_SRADIUS    25.0    /* Default photon sampling radius. */
#define XTRACER_SETUP_DEFAULT_PHOTON_POWERSC    1.25    /* Default photon power scaling factor. */
#define XTRACER_SETUP_DEFAULT_MAX_RDEPTH        3       /* Default maximum recursion depth. */
#define XTRACER_SETUP_DEFAULT_DOF_SAMPLES       10      /* Default sample count for DOF. */
#define XTRACER_SETUP_DEFAULT_LIGHT_SAMPLES     1       /* Default sample count for lights. */
#define XTRACER_SETUP_DEFAULT_REFLEC_SAMPLES    1       /* Default sample count for reflection. */

#define TILESIZE 64

using Util::String::path_comp;

Renderer::Renderer()
	: mFramebuffer(NULL)
	, mScene(NULL)
{}

void Renderer::setup(Context &context)
{
	mFramebuffer = context.framebuffer;
	mScene       = context.scene;
}

void Renderer::render()
{
	if (!mScene || !mFramebuffer) return;
	pass_ptrace();
	pass_rtrace();
}

void Renderer::pass_ptrace()
{
	if (!XTRACER_SETUP_DEFAULT_GI) return;
	if (!mScene) return;

	unsigned int photon_count = XTRACER_SETUP_DEFAULT_PHOTON_COUNT; // Environment::handle().photon_count();

	Log::handle().log_message("Initiating the photon maps..", photon_count);
	Log::handle().log_message("Using %i photons..", photon_count);
	m_pm_global.init(photon_count);
	m_pm_caustic.init(photon_count);

	Log::handle().log_message("Distributing photons to light sources..", photon_count);
	// Calculate each light's contribution by using the intensity luminance.
	// In future revisions I should also take the light source's size into consideration.
	std::vector<unsigned int> light_photons;
	{
		unsigned int light_count = mScene->m_lights.size();	// Total light count.
		std::vector<scalar_t> light_contribution;			// Vector with each light's contribution.
		std::vector<scalar_t> light_luminance;				// Vector with each light's luminance.
		scalar_t light_total_luminance = 0;					// Total light luminance.

		// Calculate the luminance for each light and the total.
		std::map<std::string, Light*>::iterator it = mScene->m_lights.begin();
		std::map<std::string, Light*>::iterator et = mScene->m_lights.end();
		for (; it != et; ++it) {
			scalar_t lum = NImg::Operator::luminance((*it).second->intensity());
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
		std::map<std::string, Light*>::iterator it = mScene->m_lights.begin();
		std::map<std::string, Light*>::iterator et = mScene->m_lights.end();
		for (; it != et; ++it) {
			while (light_photons[light_index] > 0) {
				Ray ray = (*it).second->ray_sample();
				trace_photon(ray, 0, (*it).second->intensity()
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
	if (!mScene) return false;

	if (depth > XTRACER_SETUP_DEFAULT_MAX_RDEPTH) return false;

	// Intersect.
	IntInfo info;
	memset(&info, 0, sizeof(info));
	std::string obj;

	if (!mScene->intersection(ray, info, obj)) return false;

	// Get the material & texture.
	Material *mat = mScene->m_materials[mScene->m_objects[obj]->material];
	std::map<std::string, Texture2D *>::iterator it_tex = mScene->m_textures.find(mScene->m_objects[obj]->texture);

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
		if (it_tex != mScene->m_textures.end())
			texcol = ColorRGBf((*it_tex).second->sample((float)info.texcoord.x, (float)info.texcoord.y));

		trace_photon(nray, depth + 1, power * texcol * mat->diffuse, map_capacity);

		return true;
	}

	return false;
}

void Renderer::pass_rtrace()
{
	if (!mFramebuffer) return;

	// precalculate some constants
	const unsigned int w = mFramebuffer->width();
	const unsigned int h = mFramebuffer->height();

	const unsigned int dxt       = (w % TILESIZE > 0 ? 1 : 0);
	const unsigned int dyt       = (h % TILESIZE > 0 ? 1 : 0);
    const unsigned int numxtiles = w / TILESIZE + dxt;
	const unsigned int numytiles = h / TILESIZE + dyt;
	const unsigned int numtiles  = numxtiles * numytiles;

	float progress = 0;

	// Samples per pixel, offset per sample.
	unsigned int aa = XTRACER_SETUP_DEFAULT_AA; // Environment::handle().aa();

	// Explicitely set the thread count if requested.
	const unsigned int thread_count = 0;// Environment::handle().threads();
	if (thread_count) {
		omp_set_num_threads(thread_count);
	}

	const unsigned int dof_samples = XTRACER_SETUP_DEFAULT_DOF_SAMPLES;
	float one_over_h = 1.f / numtiles;
	float spp = (float)(aa * aa);
	double subpixel_size  = 1.0f / (float)(aa);
	double subpixel_size2 = subpixel_size / 2.0f;

	// Calculate the number of samples per pixel.
	float sample_scaling = 1.0f / ((mScene->camera->flength > 0 ? dof_samples : 1.0f) * spp);

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

						if (mScene->camera->flength > 0) {
							// dof loop
							for (float dofs = 0; dofs < dof_samples; ++dofs) {
								Ray ray = mScene->camera->get_primary_ray_dof(rx , ry, (float)w, (float)h);
								color += (trace_ray(ray,XTRACER_SETUP_DEFAULT_MAX_RDEPTH + 1) * sample_scaling);
							}
						}
						else {
							Ray ray = mScene->camera->get_primary_ray(rx, ry, (float)w, (float)h);
							color += (trace_ray(ray,XTRACER_SETUP_DEFAULT_MAX_RDEPTH + 1) * sample_scaling);
						}
					}
				}

				mFramebuffer->pixel(x, y) = color;
			}
		}

		#pragma omp critical
		{
			// calculate progress
			++progress;

			std::cout.setf(std::ios::fixed, std::ios::floatfield);
			std::cout.setf(std::ios::showpoint);

			static const unsigned int length = 25;
			std::cout << "\rProgress [ ";

			float pr = progress * one_over_h;

			for (unsigned int i = 0; i < length; i++) {
				float p = pr * length;
				if		(i < p) std::cout << '=';
				else if (i - p < 1) std::cout << '>';
				else	std::cout << ' ';
			}

			int totalw = omp_get_num_threads();
			// get the string length of totalw
			int wlen = 0;
			int num = totalw;

			while (num > 0) { num /= 10; wlen++; }

			std::cout	<< " " << std::setw(6) << std::setprecision(2) << pr * 100.0
						<< "% ] [ "<< totalw << " C: " << std::setw(wlen)
						<< omp_get_thread_num() << " ]" << std::flush;
		}
	}
}

ColorRGBf Renderer::trace_ray(const Ray &ray, const unsigned int depth,
	const scalar_t ior_src, const scalar_t ior_dst)
{
	IntInfo info;
	memset(&info, 0, sizeof(info));

	ColorRGBf gi_res(0, 0, 0);

	// Check for ray intersection
	std::string obj; // this will hold the object name
	if (mScene->intersection(ray, info, obj)) {
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
					XTRACER_SETUP_DEFAULT_PHOTON_SRADIUS, //Environment::handle().photon_max_sampling_radius(),
					XTRACER_SETUP_DEFAULT_PHOTON_SAMPLES  //Environment::handle().photon_max_samples()
				);

				gi_res = ColorRGBf(irad[0], irad[1], irad[2]);

				if (XTRACER_SETUP_DEFAULT_GIVIZ) //Environment::handle().flag_giviz())
					return gi_res;
			}

			Material *mat = mScene->m_materials[mScene->m_objects[obj]->material];

			// if the ray starts inside the geometry
			scalar_t dot_normal_dir = dot(info.normal, ray.direction);
			if (mat->transparency > EPSILON && dot_normal_dir > 0) info.normal = -info.normal;
			scalar_t ior_a = dot_normal_dir > 0 ? mat->ior : ior_src;
			scalar_t ior_b = dot_normal_dir > 0 ? ior_src  : mat->ior;

			return gi_res + shade(ray, depth, info, obj, ior_a, ior_b);
		}
	}

	return ColorRGBf(0, 0, 0);
}

ColorRGBf Renderer::shade(const Ray &ray, const unsigned int depth,
	IntInfo &info, std::string &obj,
	const scalar_t ior_src, const scalar_t ior_dst)
{
	ColorRGBf color(0, 0, 0);

	// check if the depth limit was reached
	if (!depth)
		return color;

	Object *mobj = mScene->m_objects[obj];
	std::map<std::string, Texture2D *>::iterator it_tex = mScene->m_textures.find(mobj->texture);
	std::map<std::string, Material  *>::iterator it_mat = mScene->m_materials.find(mobj->material);

	if (it_mat == mScene->m_materials.end()) return color;

	// shadows
	Vector3f n = info.normal;
	Vector3f p = info.point;

	Material *mat = mScene->m_materials[mScene->m_objects[obj]->material];

	// Precalculated roughness.
	scalar_t roughness = mat->roughness;

	// ambient
	color = mat->ambient * mScene->ambient();

	scalar_t shadow_sample_scaling = 1.0f / XTRACER_SETUP_DEFAULT_LIGHT_SAMPLES;// Environment::handle().samples_light();

	std::map<std::string, Light*>::iterator it = mScene->m_lights.begin();
	std::map<std::string, Light*>::iterator et = mScene->m_lights.end();

	for (; it != et; ++it) {
		unsigned int tlshsamples = ((*it).second->is_area_light() ? XTRACER_SETUP_DEFAULT_LIGHT_SAMPLES : 1);
		scalar_t tlshscaling = ((*it).second->is_area_light() ? shadow_sample_scaling : 1.0);

		for (unsigned int shsamples = 0; shsamples < tlshsamples; ++shsamples) {
			Light *light = (*it).second;
			Vector3f v = light->point_sample() - p;

			// Texture.
			ColorRGBf texcolor = ColorRGBf(1,1,1);
			if (it_tex != mScene->m_textures.end()) {
				texcolor = ColorRGBf((*it_tex).second->sample((float)info.texcoord.x, (float)info.texcoord.y));
			}

			Ray sray;
			sray.origin = p;
			sray.direction = v.normalized();
			scalar_t distance = v.length();

			// if the point is not in shadow for this light
			std::string obj;
			IntInfo res;
			bool test = mScene->intersection(sray, res, obj);
			if (!test || res.t < EPSILON || res.t > distance) {
				// shade
				color += mat->shade(mScene->camera, light, texcolor, info) * tlshscaling;
			}
		}
	}

	// specular effects

	// Fresnel
	float fr = 1.0;
	float ft = 0.0;

	if ((mat->type == MATERIAL_PHONG) || (mat->type == MATERIAL_BLINNPHONG)) {
		// refraction
		if(mat->transparency > 0.0) {
			Ray refrray;
			refrray.origin = p;

			refrray.direction = (ray.direction).refracted(n, ior_src, ior_dst);

			float f1 = ior_src * (1.0 - dot(n, ray.direction));
			float f2 = ior_dst * (1.0 - dot(-n, refrray.direction));

			ft = pow((f1 - f2) / (f1 + f2), 2.0);
			fr = 1.0 - ft;

			color *= (1.0 - mat->transparency);
			color += ft * mat->transparency * trace_ray(refrray, depth-1, ior_src, ior_dst) * mat->specular * mat->kspec;
		}

		// reflection
		if(mat->reflectance > 0.0) {
			unsigned int tlmcsamples = XTRACER_SETUP_DEFAULT_REFLEC_SAMPLES; // Environment::handle().samples_reflection();
			scalar_t tlmcscaling = 1.0f / (scalar_t)tlmcsamples;
			for (unsigned int mcsamples = 0; mcsamples < tlmcsamples; ++mcsamples) {
				Ray reflray;
				reflray.origin = p;
				reflray.direction = NMath::Sample::lobe(n, -ray.direction, mat->roughness == 0 ? mat->ksexp : roughness);
				color += fr * (mat->reflectance * trace_ray(reflray, depth-1) * mat->specular * mat->kspec * tlmcscaling);
			}
		}
	}

	return color;
}
