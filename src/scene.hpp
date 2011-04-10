/*

    This file is part of xtracer.

    scene.hpp
    Scene class

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

#ifndef XTRACER_SCENE_HPP_INCLUDED
#define XTRACER_SCENE_HPP_INCLUDED

#include <map>

#include <nmath/ray.h>
#include <nparse/cfgparser.hpp>

#include "err.h"

#include "pixel.h"
#include "camera.hpp"
#include "light.hpp"
#include "material.hpp"
#include "geometry.hpp"
#include "spscheme.hpp"

class Scene
{
	friend class Renderer;
	protected:
		Scene(const char *filepath);
		~Scene();

		xt_status_t init();
		xt_status_t analyze();

		xt_status_t set_bgcolor();

		xt_status_t set_camera(const char *name);
		xt_status_t add_light(NCFGParser *p);
		xt_status_t add_geometry(NCFGParser *p);
		xt_status_t add_material(NCFGParser *p);

		/*
			Trace a ray
			This function will pass the arguments to the 
			space partitioning scheme before calculating
			an intermediate color
		*/
		pixel32_t trace(Ray &ray);

		/*
			Shade
		*/
		pixel32_t shade(Ray &ray, SPCRes &col);

		/*
			The space partitioning scheme
			This implementation supports the extension
			of the current system to support many different
			partition schemes by class inheritance
		*/
		SPScheme *space;

		/*
			Pointer to a camera class
			This is not static so that the camera class can be
			later on extended to provide multiple models
		*/
		Camera *camera;
		
		/*
			The scene's source file path						
		*/
		std::string source;

		/*
			Lists of the scene entities		
		*/
		std::map<std::string, Light *> light;
		std::map<std::string, Material *> material;
		std::map<std::string, Geometry *> geometry;

		/*
			The parsed tree root node
		*/
		NCFGParser data;

		unsigned int rdepth;
		pixel32_t background_color;

	private:
		void cleanup();
};

#endif /* XTRACER_SCENE_HPP_INCLUDED */
