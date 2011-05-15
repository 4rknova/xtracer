/*

    This file is part of xtracer.

    renderer.cpp
    Renderer class

    Copyright (C) 2010, 2011
    Papadopoulos Nikolaos

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General
    Public License along with this library; if not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301 USA

*/

#include <iomanip>
#include <iostream>
#include "renderer.hpp"

Renderer::Renderer(Framebuffer &fb, Scene &scene, Driver *drv, unsigned int depthlim)
	: m_fb(&fb), 
	m_scene(&scene), 
	m_drv(drv), 
	m_max_rdepth(depthlim), 
	m_verbosity(0),
	m_antialiasing(1),
	m_gamma(1),
	m_f_light_geometry(false),
	m_f_realtime_update(false)
{}

// report the progress
void rprog(float progress)
{
	static const unsigned int length = 25;

	std::cout
		<< "\rProgress [ ";
	for (unsigned int i = 0; i < length; i++)
	{
		float p = progress * length / 100;
		if (i < p) 
			std::cout << '=';
		else if (i - p < 1)
			std::cout << '>';
		else
			std::cout << ' ';
	}

	std::cout
		<< " "
		<< std::setw(6) << std::setprecision(2)
		<< progress		 
		<< "% ]"
		<< std::flush;
}

#include <nparse/parseutils.h>
#include <nmath/sphere.h>
unsigned int Renderer::render()
{
	// get the source file name
	std::string path, file;
	nstring_path_comp(m_scene->source, path, file);

	// tag the framebuffer
	m_fb->tag(file.c_str());

	// clear the framebuffer
	m_fb->clear();

	// initiate the output driver
	m_drv->init();

	// render the frame
	if (render_frame())
		return 1;

	// apply gamma correction
	if (m_gamma != 1.0)
	{
		std::cout << "Applying gamma correction: " << m_gamma << "\n";
		m_fb->apply_gamma(m_gamma);
	}

	// update the output
	m_drv->update();

	// terminate the output driver
	m_drv->deinit();
	return 0;
}

unsigned int Renderer::render_frame()
{
	std::cout << "Rendering..\n";

	// precalculate some constants
	const unsigned int w = m_fb->width();
	const unsigned int h = m_fb->height();
	float progress = 0;

	// setup the output
	std::cout.setf(std::ios::fixed, std::ios::floatfield);
	std::cout.setf(std::ios::showpoint);
			
	// antialiasing samples
	unsigned int samples_per_pixel = m_antialiasing * m_antialiasing;
	float offset_per_sample = 1.0 / m_antialiasing;

	for (float y = 0; y < (float)h; y++) 
	{
		for (float x = 0; x < (float)w; x++) 
		{
			// the final color
			Vector3 color;

			// antialiasing loop
			for (float fragmenty = y; fragmenty < y + 1.0; fragmenty += offset_per_sample)
			{
				for (float fragmentx = x; fragmentx < x + 1.0; fragmentx += offset_per_sample)
				{
					// generate primary ray and trace it
					Ray ray = m_scene->camera->get_primary_ray(fragmentx, fragmenty, w, h);
					color += trace(ray, m_max_rdepth+1) / samples_per_pixel;
				}
			}
			*(m_fb->pixel(x, y)) += color;
		}

		if (m_f_realtime_update)
			m_drv->update(0, y, w, y+1); // update the output

		// calculate progress
		progress = y / (float)(h-1) * 100;
		// report progress
		rprog(progress);
	}

	std::cout << '\n';
	return 0;
}

Vector3 Renderer::trace(const Ray &ray, unsigned int depth, real_t ior_src, real_t ior_dst)
{
	IntInfo info;
	memset(&info, 1, sizeof(info));

	// Check for ray intersection
	std::string obj; // this will hold the object name
	if (m_scene->intersection(ray, info, obj, m_f_light_geometry))
	{
		// get a pointer to the material
		Material *mat = m_scene->material[m_scene->object[obj]->material];
		// if the ray starts inside the geometry
		if (dot(info.normal, ray.direction) > 0)
		{
			info.normal = -info.normal;
			return shade(ray, depth, info, obj, mat->ior, ior_src);
		}
		else
		{
			return shade(ray, depth, info, obj, ior_src, mat->ior);
		}
	}

	return Vector3(0, 0, 0);
}

Vector3 Renderer::shade(const Ray &ray, unsigned int depth, IntInfo &info, std::string &obj, real_t ior_src, real_t ior_dst)
{
	Vector3 color;
		
	// check if the intersection geometry is a light
	if (obj.empty())
	{
		return Vector3(1, 1, 0);
	}

	// check if the depth limit was reached
	if (!depth)
		return color;

	std::map<std::string, Light *>::iterator it;
	Material *mat = m_scene->material[m_scene->object[obj]->material];
	
	// ambient
	color = mat->ambient * m_scene->k_ambient * mat->diffuse;
		
	// shadows
	Vector3 n = info.normal;
	Vector3 p = info.point;

	for (it = m_scene->light.begin(); it != m_scene->light.end(); it++)
	{
		Light *light = (*it).second;
		Vector3 v = light->position - p;

		Ray sray;
		sray.origin = p;
		sray.direction = v.normalized();
		real_t distance = v.length();

		// if the point is not in shadow for this light
		std::string obj;
		IntInfo res;
		bool test = m_scene->intersection(sray, res, obj);
		if (!test || res.t < EPSILON || res.t > distance) 
		{
			// shade
			color += mat->shade(m_scene->camera, light, info);
		}
	}

	// specular effects
	if ((mat->type == MATERIAL_PHONG) || (mat->type == MATERIAL_BLINNPHONG))
	{
		// reflection
		if(mat->reflectance > 0.0)
		{
			Ray reflray;
			reflray.origin = p;
			reflray.direction = (-ray.direction).reflected(n);
			color += mat->reflectance * trace(reflray, depth-1) * mat->specular;
		}

		// refraction
		if(mat->transparency > 0.0)
		{
			Ray refrray;
			refrray.origin = p;

			refrray.direction = (ray.direction).refracted(n, ior_src, ior_dst);

			color *= (1.0 - mat->transparency);
			color += mat->transparency * trace(refrray, depth-1, ior_src, ior_dst) * mat->specular;
		}
	}

	return color;
}

unsigned int Renderer::verbosity(int v)
{
	if(v < 0)
		return m_verbosity;
	return m_verbosity = v;
}

bool Renderer::realtime_update(int v)
{
	if (v < 0)
		return m_f_realtime_update;
	return m_f_realtime_update = (v == 0 ? false : true);

}

bool Renderer::light_geometry(int v)
{
	if (v < 0)
		return m_f_light_geometry;
	return m_f_light_geometry = (v == 0 ? false : true);
}

real_t Renderer::gamma_correction(real_t v)
{
	if (v < 0)
		return m_gamma;
	return m_gamma = v;
}

unsigned int Renderer::max_recursion_depth(int v)
{
	if (v < 0)
		return m_max_rdepth;
	return m_max_rdepth = v;
}

unsigned int Renderer::antialiasing(int v)
{
	if (v < 1)
		return m_antialiasing;
	return m_antialiasing = v;
}
