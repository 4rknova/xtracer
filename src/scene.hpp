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

#include <list>

#include <nparse/cfgparser.hpp>

#include "err.h"
#include "light.hpp"
#include "material.hpp"
#include "camera.hpp"
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

		xt_status_t set_camera(const char *name);
		xt_status_t add_light(NCFGParser *p);
		xt_status_t add_geometry(NCFGParser *p);
		xt_status_t add_material(NCFGParser *p);

		/*
			Pointer to a camera class
			This is not static so that the camera class can be
			later on extended to provide multiple models
		*/
		Camera *camera;

		/*
			Lists of the scene entities		
		*/
		std::list<Light *> light;
		std::list<Geometry *> geometry;
		std::list<Material *> material;

		/*
			The parsed tree root node
		*/
		NCFGParser data;

		/*
			The scene's source file path						
		*/
		std::string source;


		/*
			The space partitioning scheme
			This implementation supports the extension
			of the current system to support many different
			partition schemes by class inheritance
		*/
		SPScheme *space;

	private:
		xt_status_t cleanup();
};

#endif /* XTRACER_SCENE_HPP_INCLUDED */