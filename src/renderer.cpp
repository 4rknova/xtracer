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

Renderer::Renderer(Framebuffer &fb, Scene &scene)
	: m_fb(&fb), m_scene(&scene)
{}

// report the progress
void rprog(float progress)
{
	static const unsigned int length = 25;

	std::cout
		<< "\rProgress [ ";
	for (int i=0; i<length; i+=1)
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

unsigned int Renderer::render()
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
			// render

			// calculate progress
			progress = (y * w + x) / (float)(pixel_count) * 100;
		}
		rprog(progress);
	}

	std::cout << '\n';
	return 0;
}

pixel32_t Renderer::trace(const Ray &ray, int depth)
{


}
