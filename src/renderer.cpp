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
	: m_fb(&fb), m_scene(&scene), m_drv(drv), max_depth(depthlim)
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

			// calculate progress
			progress = (y * w + x) / (float)(pixel_count) * 100;
		}

		rprog(progress);
	}

	std::cout << '\n';
	return 0;
}

Vector3 Renderer::trace(const Ray &ray, unsigned int depth)
{
	IntInfo info;

	// Check for ray intersection
	if (m_scene->intersection(ray, &info))
	{
		return shade(ray, depth, &info);
	}

	return Vector3(0, 0, 0);
}

Vector3 Renderer::shade(const Ray &ray, unsigned int depth, IntInfo *info)
{
	// check if the depth limit was reached
	if (!depth)
		return Vector3(0, 0, 0);

	Vector3 normal = info->normal;
	Vector3 ipoint = info->point;
	Vector3 v = (ray.origin - ipoint).normalized();

	// get material here



	Vector3 color = m_scene->ambient;

	return color;
}
