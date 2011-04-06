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

#include "renderer.hpp"

#include "pixel.h"

Renderer::Renderer(const char *filepath, Framebuffer &fb, unsigned int depth):
	m_p_scene(filepath), m_p_fb(&fb), m_p_depth(depth)
{}

unsigned int Renderer::recursion_depth()
{
	return m_p_depth;
}

unsigned int Renderer::set_recursion_depth(unsigned int depth)
{
	return m_p_depth = depth;
}

#include <stdint.h>

xt_status_t Renderer::render(const char *camera)
{
	if(m_p_fb == NULL)
	{
		fprintf(stderr, "Error: Invalid framebuffer (NULL).\n");
		return XT_STATUS_FB_INVALID;
	}

	xt_status_t status = XT_STATUS_OK;
	
	/* Initiate the scene */
	status = m_p_scene.init();

	if(status != XT_STATUS_OK)
	{
		return status;
	}

	/* Set up the camera */
	m_p_scene.set_camera(camera);

	/* Render */
	printf("Rendering frame..\n");

	/* Rendering loop */
	float total_pixels =  m_p_fb->height() *  m_p_fb->width();
	for (unsigned int h = 0; h < m_p_fb->height(); h++)
	{
		for (unsigned int w = 0; w < m_p_fb->width(); w++)
		{
			if (!m_p_scene.camera)
				break;

			Ray primary = m_p_scene.camera->get_primary_ray(w, h, m_p_fb->width(), m_p_fb->height());
if((primary.direction.z >.9999)){
//			printf("origin: %3.3f %3.3f %3.3f direction: %3.3f %3.3f %3.3f len:%f \n", primary.origin.x, primary.origin.y, primary.origin.z, primary.direction.x, primary.direction.y, primary.direction.z, primary.direction.length());
//getchar();
}
			real_t depth=0;


			for (std::list<Geometry *>::iterator it = m_p_scene.geometry.begin(); it != m_p_scene.geometry.end(); it++)
			{
				real_t pdepth = (*it)->collision(primary) * 20;

				if (pdepth < NM_INFINITY)
				{
					depth = (((float)pdepth)/255.0f) > 254 ? 255 : pdepth;
				}
			}

			uint32_t final_color = rgba_to_pixel32(0, 0, (char)depth, 255);


			m_p_fb->set_pixel(w, h, final_color);

			if (total_pixels > 1)
			{
				printf("\rProgress: %06.2f%% of %i pixels.", 
					((h * m_p_fb->width() + w)/(total_pixels - 1))*100, 
					(int)total_pixels);
			}
			std::cout << std::flush;
		}
	}
	printf("\nDone\n");
	return XT_STATUS_OK;
}
