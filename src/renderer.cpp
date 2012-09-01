/*

    This file is part of xtracer.

    renderer.cpp
    Renderer class

    Copyright (C) 2010, 2011
    Papadopoulos Nikolaos

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General
    Public License along with this program; if not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301 USA

*/

#include <vector>
#include <iomanip>
#include <omp.h>
#include <ncf/util.hpp>
#include <nmath/mutil.h>
#include <nmath/prng.h>
#include <nmath/sample.h>
#include <nmath/plane.h>
#include <nimg/luminance.hpp>
#include "setup.h"
#include "object.hpp"
#include "argparse.hpp"
#include "log.hpp"
#include "console.hpp"
#include "photonmap.hpp"
#include "renderer.hpp"

using NCF::Util::path_comp;

Renderer::Renderer()
{}
#include <nmath/interpolation.h>
void Renderer::render(Framebuffer &fb, Scene &scene)
{
	// If gi is enabled, do the photon mapping 1st pass.
	if (Environment::handle().flag_gi()) {
		Log::handle().log_message("Global illumination is enabled.");
		pass_ptrace(scene);
	}

	// Render the frame.
	pass_rtrace(fb, scene);
}

void Renderer::pass_ptrace(Scene &scene)
{
	unsigned int photon_count = Environment::handle().photon_count();
	
	Log::handle().log_message("Initiating the photon maps..", photon_count);
	Log::handle().log_message("Using %i photons..", photon_count);
	scene.m_pm_global.init(photon_count);
	scene.m_pm_caustic.init(photon_count);

	Log::handle().log_message("Distributing photons to light sources..", photon_count);
	// Calculate each light's contribution by using the intensity luminance.
	// In future revisions I should also take the light source's size into consideration.
	std::vector<unsigned int> light_photons;
	{
		unsigned int light_count = scene.m_lights.size();	// Total light count.
		std::vector<scalar_t> light_contribution;			// Vector with each light's contribution.
		std::vector<scalar_t> light_luminance;				// Vector with each light's luminance.
		scalar_t light_total_luminance = 0;					// Total light luminance.

		// Calculate the luminance for each light and the total.
		for (std::map<std::string, Light*>::iterator it = scene.m_lights.begin(); it != scene.m_lights.end(); ++it) {
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

	// Photon tracing.
	unsigned int light_index = 0;
	for (std::map<std::string, Light*>::iterator it = scene.m_lights.begin(); it != scene.m_lights.end(); ++it) {
		while (light_photons[light_index] > 0) {
			Ray ray = (*it).second->ray_sample();
			trace_photon(scene, ray, 0, (*it).second->intensity() * Environment::handle().photon_power_scaling(), light_photons[light_index]);
		}
		
		light_index++;
	}

	Log::handle().log_message("Balancing the photon map..");
	scene.m_pm_global.balance();
}

bool Renderer::trace_photon(Scene &scene, const Ray &ray, const unsigned int depth, const ColorRGBf power, unsigned int &map_capacity)
{
	if (depth > Environment::handle().max_rdepth())
		return false;

	// Intersect.
	IntInfo info;
	memset(&info, 0, sizeof(info));
	std::string obj;

	if (!scene.intersection(ray, info, obj))
		return false;
	
	// Get the material.
	Material *mat = scene.m_materials[scene.m_objects[obj]->material];
	
	// Check if there are photons left to consume.
	if (depth > 0 &&  map_capacity > 0) {
		// Store the photon.
		float pos[3]; // Photon position.
		float pwr[3]; // Photon intensity.
		float dir[3]; // Photon direction.
		
		pwr[0] = power.r(); 		pwr[1] = power.g(); 		pwr[2] = power.b();
		pos[0] = ray.origin.x;		pos[1] = ray.origin.y;		pos[2] = ray.origin.z;
		dir[0] = -ray.direction.x; 	dir[1] = -ray.direction.y; 	dir[2] = -ray.direction.z;

		scene.m_pm_global.store(pos, pwr, dir);
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
		trace_photon(scene, nray, depth + 1, power * mat->diffuse, map_capacity);

		return true;
	}

	return false;
}

void Renderer::pass_rtrace(Framebuffer &fb, Scene &scene)
{
	// precalculate some constants
	const unsigned int rminx = Environment::handle().region_min_x();
	const unsigned int rminy = Environment::handle().region_min_y();
	const unsigned int rmaxx = Environment::handle().region_max_x();
	const unsigned int rmaxy = Environment::handle().region_max_y();
	const unsigned int width = fb.width();
	const unsigned int height = fb.height();
	const unsigned int w = (rmaxx > 0 && rmaxx < width ? rmaxx : width);
	const unsigned int h = (rmaxy > 0 && rmaxy < height ? rmaxy : height);

	if ( rminx != 0 && rminy != 0 && w != width && h != height) {
		Log::handle().log_message("-> Using regional boundaries: %ix%i - %ix%i", rminx, rminy, w, h);
	}

	Log::handle().log_message("Rendering..");
	Log::handle().log_message("Progress ...");

	float progress = 0;
			
	// Samples per pixel, offset per sample.
	unsigned int aa = Environment::handle().aa();

	// Explicitely set the thread count if requested.
	const unsigned int thread_count = Environment::handle().threads();
	if (thread_count) {
		omp_set_num_threads(thread_count);
	}

	const unsigned int dof_samples = Environment::handle().samples_dof();
	float one_over_h = 1.f / (float)(h - rminy > 0 ? h - rminy : 1) * 100.f;
	float spp = (float)(aa * aa);
	double subpixel_size  = 1.0f / (float)(aa);
	double subpixel_size2 = subpixel_size / 2.0f;

	// Calculate the number of samples per pixel.
	float sample_scaling = 1.0f / ((scene.camera->flength > 0 ? dof_samples : 1.0f) * spp);

	#pragma omp parallel for schedule(dynamic, 1)
	for (int y = rminy; y < (int)h; y++) 
	{
		for (int x = rminx; x < (int)w; x++)
		{
			// the final color
			ColorRGBf color;

			// antialiasing loop
			for (unsigned int fy = 0; fy < aa; ++fy) {
				for (unsigned int fx = 0; fx < aa; ++fx) {
					
					float rx = (float)x + (float)fx * subpixel_size + subpixel_size2;
					float ry = (float)y + (float)fy * subpixel_size + subpixel_size2;

					if (scene.camera->flength > 0) {
						// dof loop
						for (float dofs = 0; dofs < dof_samples; ++dofs) {
							Ray ray = scene.camera->get_primary_ray_dof(rx , ry, (float)width, (float)height);
							color += (trace_ray(scene, ray, Environment::handle().max_rdepth() + 1) * sample_scaling);
						}
					}
					else {
						Ray ray = scene.camera->get_primary_ray(rx, ry, (float)width, (float)height);
						color += (trace_ray(scene, ray, Environment::handle().max_rdepth() + 1) * sample_scaling);
					}
				}
			}

			fb.pixel(x, y) = color;
		}
		
		// calculate progress
		progress += 1;

		#pragma omp critical
		{
			Console::handle().progress(progress * one_over_h, omp_get_thread_num(), omp_get_num_threads());
		}
	}
}

ColorRGBf Renderer::trace_ray(Scene &scene, const Ray &ray, const unsigned int depth, 
	const scalar_t ior_src, const scalar_t ior_dst)
{
	IntInfo info;
	memset(&info, 0, sizeof(info));

	ColorRGBf gi_res(0, 0, 0);

	// Check for ray intersection
	std::string obj; // this will hold the object name
	if (scene.intersection(ray, info, obj)) {
		// get a pointer to the material
		if (!obj.empty()) {

			if (Environment::handle().flag_gi()) {			
				float irad[3] = {0,0,0};
				float posi[3] = {(float)info.point.x, (float)info.point.y, (float)info.point.z};
		
				float norm[3];
				norm[0] = (float)info.normal.x;
				norm[1] = (float)info.normal.y; 
				norm[2] = (float)info.normal.z;

				scene.m_pm_global.irradiance_estimate(irad, posi, norm, 
					Environment::handle().photon_max_sampling_radius(), 
					Environment::handle().photon_max_samples());

				gi_res = ColorRGBf(irad[0], irad[1], irad[2]);

				if (Environment::handle().flag_giviz())
					return gi_res;
			}			
		
			Material *mat = scene.m_materials[scene.m_objects[obj]->material];

			// if the ray starts inside the geometry
			scalar_t dot_normal_dir = dot(info.normal, ray.direction);
			if (mat->transparency > EPSILON && dot_normal_dir > 0) info.normal = -info.normal;
			scalar_t ior_a = dot_normal_dir > 0 ? mat->ior : ior_src;
			scalar_t ior_b = dot_normal_dir > 0 ? ior_src  : mat->ior;

			return gi_res + shade(scene, ray, depth, info, obj, ior_a, ior_b);
		}
	}

	return ColorRGBf(0, 0, 0);
}

ColorRGBf Renderer::shade(Scene &scene, const Ray &ray, const unsigned int depth, 
	IntInfo &info, std::string &obj, 
	const scalar_t ior_src, const scalar_t ior_dst)
{
	ColorRGBf color(0, 0, 0);
		
	// check if the depth limit was reached
	if (!depth)
		return color;
	
	std::map<std::string, Texture2D *>::iterator it_tex = scene.m_textures.find(scene.m_objects[obj]->texture);
	std::map<std::string, Material  *>::iterator it_mat = scene.m_materials.find(scene.m_objects[obj]->material);
	std::map<std::string, Light     *>::iterator it;

	if (it_mat == scene.m_materials.end()) {
		return color;
	}

	// shadows
	Vector3f n = info.normal;
	Vector3f p = info.point;

	Material *mat = scene.m_materials[scene.m_objects[obj]->material];

	// Precalculated roughness.
	scalar_t roughness = 1000000.0 / mat->roughness;

	// ambient
	color = mat->ambient * scene.ambient();

	scalar_t shadow_sample_scaling = 1.0f / Environment::handle().samples_light();

	for (it = scene.m_lights.begin(); it != scene.m_lights.end(); it++) {
		unsigned int tlshsamples = ((*it).second->is_area_light() ? Environment::handle().samples_light() : 1);
		scalar_t tlshscaling = ((*it).second->is_area_light() ? shadow_sample_scaling : 1.0);

		for (unsigned int shsamples = 0; shsamples < tlshsamples; ++shsamples) {
			Light *light = (*it).second;
			Vector3f v = light->point_sample() - p;
	
			// Texture.
			ColorRGBf texcolor = ColorRGBf(1,1,1);
			if (it_tex != scene.m_textures.end()) {
				texcolor = ColorRGBf((*it_tex).second->sample((float)info.texcoord.x, (float)info.texcoord.y));
			}

			Ray sray;
			sray.origin = p;
			sray.direction = v.normalized();
			scalar_t distance = v.length();

			// if the point is not in shadow for this light
			std::string obj;
			IntInfo res;
			bool test = scene.intersection(sray, res, obj);
			if (!test || res.t < EPSILON || res.t > distance) {
				// shade
				color += mat->shade(scene.camera, light, texcolor, info) * tlshscaling;
			}
		}
	}
	
	// specular effects
	if ((mat->type == MATERIAL_PHONG) || (mat->type == MATERIAL_BLINNPHONG)) {
		// refraction
		if(mat->transparency > 0.0) {
			Ray refrray;
			refrray.origin = p;

			refrray.direction = (ray.direction).refracted(n, ior_src, ior_dst);

			color *= (1.0 - mat->transparency);
			color += mat->transparency * trace_ray(scene, refrray, depth-1, ior_src, ior_dst) * mat->specular * mat->kspec;
		}

		// reflection
		if(mat->reflectance > 0.0) {
			unsigned int tlmcsamples = Environment::handle().samples_reflection();
			scalar_t tlmcscaling = 1.0f / (scalar_t)tlmcsamples;
			for (unsigned int mcsamples = 0; mcsamples < tlmcsamples; ++mcsamples) {
				Ray reflray;
				reflray.origin = p;
				reflray.direction = NMath::Sample::lobe(n, -ray.direction, mat->roughness == 0 ? mat->ksexp : roughness);
				color += (mat->reflectance * trace_ray(scene, reflray, depth-1) * mat->specular * mat->kspec * tlmcscaling);
			}
		}
	}

	return color;
}
