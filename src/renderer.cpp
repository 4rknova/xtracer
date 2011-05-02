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
	max_rdepth(depthlim), 
	m_verbosity(0), 
	m_gamma(1),
	m_f_light_geometry(false)
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

#include <nparse/parseutils.hpp>
#include <nmath/sphere.h>
unsigned int Renderer::render()
{
	// get the source file name
	std::string path, file;
	nstring_path_comp(m_scene->source, path, file);

	// tag the framebuffer
	m_fb->tag(file.c_str());

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

	for (unsigned int y = 0; y < h; y++) 
	{
		for (unsigned int x = 0; x < w; x++) 
		{
			// generate primary ray and trace it
			Ray ray = m_scene->camera->get_primary_ray(x, y, w, h);
			Vector3 color = trace(ray, max_rdepth);
			
			*(m_fb->pixel(x, y)) += color;
		}

		// calculate progress
		progress = y / (float)(h-1) * 100;

		// report progress
		rprog(progress);
	}

	std::cout << '\n';
	return 0;
}

Vector3 Renderer::trace(const Ray &ray, unsigned int depth)
{
	IntInfo info;
	memset(&info, 1, sizeof(info));

	// Check for ray intersection
	std::string obj; // this will hold the object name
	if (m_scene->intersection(ray, info, obj, m_f_light_geometry))
	{
		return shade(ray, depth, info, obj);
	}

	return Vector3(0, 0, 0);
}

#include "matlambert.hpp"

Vector3 Renderer::shade(const Ray &ray, unsigned int depth, IntInfo &info, std::string &obj)
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
	color = m_scene->ambient * m_scene->k_ambient * mat->diffuse;
		
	// calculate shadows
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

	// reflection
	if( mat->reflectance > 0.0)
	{
		Ray reflray;
		reflray.origin = p;
		reflray.direction = (ray.direction).reflected(n);
		color += mat->reflectance * trace(reflray, depth-1);
	}

	return color;
}

unsigned int Renderer::verbosity(int v)
{
	if(v < 0)
		return m_verbosity;
	return m_verbosity = v;
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

unsigned int Renderer::max_recursion_depth(unsigned int v)
{
	return max_rdepth = v;
}
