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

#include <nmath/ray.h>
#include <nmath/intinfo.h>

#include "fb.hpp"
#include "scene.hpp"

#include "pixel.h"

class Renderer
{
	public:
		Renderer(Framebuffer &fb, Scene &scene);

		// renders the scene
		unsigned int render();

	protected:
		pixel32_t trace(const Ray &ray, int depth);
		pixel32_t shade(const Ray &ray, int depth, IntInfo *info);

	private:
		// pointer to the framebuffer
		Framebuffer *m_fb;
		// pointer to the scene
		Scene *m_scene;
};

#endif /* XTRACER_RENDERER_HPP_INCLUDED */
