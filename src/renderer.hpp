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
		Renderer(Framebuffer &fb, Scene &scene, Driver *drv, 
			unsigned int depth = XT_SETUP_DEFAULT_RDEPTH );

		// render the scene
		unsigned int render();

		void set_window(unsigned int x0, unsigned int x1,
			unsigned int y0, unsigned int y1);		// Limit the rendering area

		real_t gamma_correction(real_t v=-1);		// set / get the gamma correction
		real_t exposure(real_t v=0);				// set / get the exposure correction
	
		unsigned int dof_samples(int v=0);			// set / get the level of dof samples
		bool light_geometry(int v=-1);				// set / get the light geometry flag
		bool realtime_update(int v=-1);				// set / get the realtime output update flag
		unsigned int max_recursion_depth(int v=-1);	// set / get the maximum recursion depth
		unsigned int antialiasing(int v=-1);		// set / get antialiasing
		unsigned int threads(int v=0);				// set / get the thread count

	protected:
		// render the current frame
		unsigned int render_frame();
		// trace ray
		Vector3 trace(const Ray &ray, unsigned int depth, 
			real_t ior_src = 1.0, real_t ior_dst = 1.0);
		// shade
		Vector3 shade(const Ray &ray, unsigned int depth, 
			IntInfo &info, 
			std::string &obj, 
			real_t ior_src = 1.0, real_t ior_dst = 1.0);

	private:
		// external entities
		Framebuffer *m_fb;	// pointer to the framebuffer
		Scene *m_scene;		// pointer to the scene
		Driver *m_drv;		// pointer to the output driver

		// environment
		unsigned int m_max_rdepth;		// recursion depth limit
		unsigned int m_antialiasing;	// antialiasing
		
		real_t m_gamma;					// gamma correction
		real_t m_exposure;				// exposure

		unsigned int m_dof_samples;		// dof samples

		unsigned int m_threads;			// threads count

		// flags
		bool m_f_light_geometry;  // if true, light sources will be treated as spheres
		bool m_f_realtime_update; // if true, the output will be updated at real time
};

#endif /* XTRACER_RENDERER_HPP_INCLUDED */
