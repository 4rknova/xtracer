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
	: m_fb(&fb), m_scene(&scene), m_drv(drv), max_depth(depthlim), m_verbosity(0)
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
	// initiate the output driver
	m_drv->init();

	// get the source file name
	std::string path, file;
	nstring_path_comp(m_scene->source, path, file);
	// tag the framebuffer
	m_fb->tag(file.c_str());
	
	// render the frame
	if (render_frame())
		return 1;

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
	const unsigned int pixel_count = (w * h) - 1;
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
			Vector3 color = trace(ray, max_depth);

			// correct the color
			color.x = color.x > 1.0 ? 1.0 : color.x;
			color.y = color.y > 1.0 ? 1.0 : color.y;
			color.z = color.z > 1.0 ? 1.0 : color.z;

			// convert to pixel and update the framebuffer
			color *= 255;
			pixel32_t pixel = rgba_to_pixel32(
				(char)color.x, 
				(char)color.y, 
				(char)color.z, 
				255);

			m_fb->set_pixel(x, y, pixel);
			progress = (y * w + x) / (float)(pixel_count) * 100;
		}

		// calculate progress
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
	if (m_scene->intersection(ray, info, obj))
	{
		return shade(ray, depth, info, obj);
	}

	return Vector3(0, 0, 0);
}

Vector3 Renderer::shade(const Ray &ray, unsigned int depth, IntInfo &info, std::string &obj)
{
	Vector3 color = Vector3(0, 0, 0);
	
	// check if the depth limit was reached
	if (!depth)
		return color;


	// calculate shadows
	Vector3 n = info.normal;
	Vector3 p = info.point;
	Vector3 v = (ray.origin - p).normalized();

	std::map<std::string, Light *>::iterator it;
	Material *mat = m_scene->material[m_scene->object[obj]->material];

	color = mat->diffuse * m_scene->ambient;
	for (it = m_scene->light.begin(); it != m_scene->light.end(); it++)
	{
		Light *light = (*it).second;

		// shade
		color += mat->shade(light, info);
	}

	return color;
}

unsigned int Renderer::verbosity(int v)
{
	if(v < 0)
		return m_verbosity;
	return m_verbosity = v;
}
