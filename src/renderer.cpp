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

#include <omp.h>
#include <iomanip>
#include <ncf/util.hpp>
#include <nmath/sample.h>
#include <nmath/plane.h>
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

void Renderer::render(Framebuffer &fb, Scene &scene)
{
	if(Environment::handle().flag_gi()) {
		Log::handle().log_message("Global illumination enabled.");
		photon_pass(scene);
	}

	render_frame(fb, scene);
}

void Renderer::photon_pass(Scene &scene)
{
	unsigned int photon_count = Environment::handle().photon_count();

	Log::handle().log_message("Pass 1: Tracing %i photons", photon_count);

	scene.m_pm_global.init(photon_count);

	Log::handle().log_message("Progress ...");
	
	unsigned int stored_photon_count = 0;

	float one_over_total = 100.0f / (float)photon_count;

	while (stored_photon_count < photon_count) {
		std::map<std::string, Light*>::iterator it;

		// Pick a light at random.
		unsigned int curl = (unsigned int) NMath::prng_c(0, (double)(scene.m_lights.size() - 1));

//		scene.m_lights[curl];

//		std::cout << curl << std::endl;
/*

		for (it = scene.m_lights.begin(); it != scene.m_lights.end(); it++) {}
			stored_photon_count++;

			float position[3] = {(float)NMath::prng_c(0, 1000), (float)NMath::prng_c(0, 1000), (float)NMath::prng_c(0, 1000)};
			float power[3] = {1, 1, 1};
			float direction[3] = {0, 1, 0};

			scene.m_pm_global.store(position, power, direction);
*/

		#pragma omp critical
		{
			Console::handle().progress((float)stored_photon_count * one_over_total, 1, 0);
		}
		
	}

	Log::handle().log_message("Balancing the photon map..");
	scene.m_pm_global.balance();
}

bool Renderer::trace_photon(Scene &scene, const Ray &ray, unsigned int depth)
{
	return false;
}

void Renderer::render_frame(Framebuffer &fb, Scene &scene)
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

	Log::handle().log_message("Rendering region: %ix%i - %ix%i", rminx, rminy, w, h);
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

ColorRGBf Renderer::trace_ray(Scene &scene, const Ray &ray, unsigned int depth, scalar_t ior_src, scalar_t ior_dst)
{
	IntInfo info;
	memset(&info, 0, sizeof(info));

	// Check for ray intersection
	std::string obj; // this will hold the object name
	if (scene.intersection(ray, info, obj)) {
		// get a pointer to the material
		if (!obj.empty()) {
			Material *mat = scene.m_materials[scene.m_objects[obj]->material];
			// if the ray starts inside the geometry
			if (dot(info.normal, ray.direction) > 0) {
				info.normal = -info.normal;
				return shade(scene, ray, depth, info, obj, mat->ior, ior_src);
			}
			else {
				return shade(scene, ray, depth, info, obj, ior_src, mat->ior);
			}
		}
	}

	return ColorRGBf(0, 0, 0);
}

ColorRGBf Renderer::shade(Scene &scene, const Ray &ray, unsigned int depth, IntInfo &info, std::string &obj, scalar_t ior_src, scalar_t ior_dst)
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
