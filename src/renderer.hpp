/*

    This file is part of xtracer.

    renderer.hpp
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

#ifndef XTRACER_RENDERER_HPP_INCLUDED
#define XTRACER_RENDERER_HPP_INCLUDED

#include <nimg/color.hpp>
#include <nimg/framebuffer.hpp>
#include <nmath/precision.h>
#include <nmath/vector.h>
#include <nmath/ray.h>
#include <nmath/intinfo.h>
#include "scene.hpp"

using NImg::ColorRGBf;
using NImg::Framebuffer;

class Renderer
{
	public:
		Renderer();
		void render(Framebuffer &fb, Scene &scene);

	private:
		// Pass functions.
		void pass_ptrace(Scene &scene);						// Rendering pass: Photon tracing.
		void pass_rtrace(Framebuffer &fb, Scene &scene);	// Rendering pass: Ray tracing.

		// Trace functions.
		bool trace_photon(Scene &scene, const Ray &ray, const unsigned int depth, 
						  const ColorRGBf power, unsigned int &map_capacity);

		ColorRGBf trace_ray(Scene &scene, const Ray &ray, const unsigned int depth,
							const scalar_t ior_src = 1.0, const scalar_t ior_dst = 1.0);

		// Shading.
		ColorRGBf shade(Scene &scene, const Ray &ray, const unsigned int depth, 
						IntInfo &info, std::string &obj,
						const scalar_t ior_src = 1.0, const scalar_t ior_dst = 1.0);
};

#endif /* XTRACER_RENDERER_HPP_INCLUDED */
