/*

	This file is part of xtracer.

	renderer.cpp
	Renderer

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

#include <cstdio>
#include <iostream>
#include <iomanip>

#include "renderer.hpp"

#include "pixel.h"

Renderer::Renderer(const char *filepath, Framebuffer &fb, unsigned int depth):
	m_p_scene(filepath), m_p_fb(&fb), m_p_depth(depth)
{}

unsigned int Renderer::recursion_depth()
{
	return m_p_scene.rdepth;
}

unsigned int Renderer::set_recursion_depth(unsigned int depth)
{
	return m_p_scene.rdepth = depth;
}

pixel32_t Renderer::render(const char *camera)
{
	if(m_p_fb == NULL)
	{
		fprintf(stderr, "Error: Invalid framebuffer (NULL).\n");
		return XT_STATUS_FB_INVALID;
	}

	/* Initiate the scene */
	xt_status_t status = XT_STATUS_OK;
	if((status = m_p_scene.init()) != XT_STATUS_OK)
	{
		return status;
	}

	/* Set up the camera */
	m_p_scene.set_camera(camera);

	/* Render */
	printf("Rendering frame..\n");

	/* Rendering loop */
	unsigned int pixel_count =  m_p_fb->height() *  m_p_fb->width();

	for (unsigned int h = 0; h < m_p_fb->height(); h++)
	{
		for (unsigned int w = 0; w < m_p_fb->width(); w++)
		{
			/* Create a primary ray */
			Ray primary = m_p_scene.camera->get_primary_ray(w, h, m_p_fb->width(), m_p_fb->height());

			/* Set the final color in the buffer */
			m_p_fb->set_pixel(w, h, m_p_scene.trace(primary));
			/* Report the progress */
			if (pixel_count > 1)
			{
				std::cout 
					<< "\rProgress: " 
					<< std::setw(7) << std::setprecision(3) 
					<< ((h * m_p_fb->width() + w) / (float)(pixel_count - 1)) * 100
					<< " of " 
					<< pixel_count 
					<< " pixels";
			}
			std::cout << std::flush;
		}
	}

	std::cout << "\n";
	return XT_STATUS_OK;
}
