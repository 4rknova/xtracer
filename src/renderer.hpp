/*

    This file is part of xtracer.

    renderer.hpp
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

#ifndef XTRACER_RENDERER_HPP_INCLUDED
#define XTRACER_RENDERER_HPP_INCLUDED

#include <nmath/precision.h>
#include <nmath/vector.h>
#include <nmath/ray.h>
#include <nmath/intinfo.h>

#include "fb.hpp"
#include "scene.hpp"
#include "drv.hpp"

#define XT_SETUP_DEFAULT_RDEPTH 2 

class Renderer
{
	public:
		Renderer(Framebuffer &fb, Scene &scene, Driver *drv, unsigned int depth = XT_SETUP_DEFAULT_RDEPTH );

		// renders the scene
		unsigned int render();
		// set / get verbosity level ( v < 0 returns status without changing it)
		unsigned int verbosity(int v=-1);

		// set / get the light geometry flag
		bool light_geometry(int v=-1);

		// set / get the gamma correction
		real_t gamma_correction(real_t v=-1);

		// set / get the maximum recursion depth
		unsigned int max_recursion_depth(int v=-1);

	protected:
		// renders the current frame
		unsigned int render_frame();
		Vector3 trace(const Ray &ray, unsigned int depth);
		Vector3 shade(const Ray &ray, unsigned int depth, IntInfo &info, std::string &obj);

	private:
		// pointer to the framebuffer
		Framebuffer *m_fb;
		// pointer to the scene
		Scene *m_scene;
		// pointer to the output driver
		Driver *m_drv;

		// recursion depth limit
		unsigned int m_max_rdepth;

		// verbosity
		unsigned int m_verbosity;

		// gamma correction
		real_t m_gamma;

		// flags
		bool m_f_light_geometry; // if true, light sources will be treated as spheres
};

#endif /* XTRACER_RENDERER_HPP_INCLUDED */
