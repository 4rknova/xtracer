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

#include <string>
#include <map>

#include <nmath/intinfo.h>
#include <nmath/vector.h>
#include <nmath/geometry.h>
#include <nparse/cf1.hpp>

#include "camera.hpp"
#include "light.hpp"
#include "material.hpp"
#include "object.hpp"

class Scene
{
	public:
		Scene(const char *filepath);
		~Scene();

		unsigned int init();		// Initiates the scene
		unsigned int analyze();		// Outputs a tree representation of the scene

		unsigned int set_ambient();
		unsigned int set_camera(const char *name);

		unsigned int add_light(NCF1 *p);
		unsigned int add_material(NCF1 *p);
		unsigned int add_geometry(NCF1 *p);
		
		unsigned int add_object(NCF1 *p);

		bool intersection(const Ray &ray, IntInfo &info, std::string &obj, bool lights=false);

		// The camera
		Camera *camera;

		// Maps of the scene entities		
		std::map<std::string, Light *> light;
		std::map<std::string, Material *> material;
		std::map<std::string, Geometry *> geometry;
		std::map<std::string, Object *> object;

		// Ambient
		Vector3 ambient;	// intensity
		real_t k_ambient;	// ratio


		// The scene's source filepath and filename
		const std::string source;

	private:
		// The parser data tree
		NCF1 data;

		// This will cleanup all the allocated memory
		void cleanup();
};

#endif /* XTRACER_SCENE_HPP_INCLUDED */
