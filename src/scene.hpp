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
#include <nmath/vector.h>
#include <nparse/cfgparser.hpp>

#include "pixel.h"

/*
#include "camera.hpp"
#include "light.hpp"
#include "material.hpp"
#include "geometry.hpp"
*/

class Scene
{
	public:
		Scene(const char *filepath);
		~Scene();

		unsigned int init();
		unsigned int analyze();

		unsigned int set_ambient();
		unsigned int set_camera(const char *name);

		unsigned int add_light(NCFGParser *p);
		unsigned int add_geometry(NCFGParser *p);
		unsigned int add_material(NCFGParser *p);

		// The camera
		Camera *camera;

		// Lists of the scene entities		
/*
		std::map<std::string, Light *> light;
		std::map<std::string, Material *> material;
		std::map<std::string, Geometry *> geometry;
*/
		// Recursion depth
		unsigned int rdepth;

		// Background color
		Vector3 ambient;

	private:
		// The scene's source filepath and filename
		std::string source;

		// The parser data tree
		NCFGParser data;

		// This will cleanup all the allocated memory
		void cleanup();
};

#endif /* XTRACER_SCENE_HPP_INCLUDED */
