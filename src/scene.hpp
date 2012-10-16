/*

    This file is part of xtracer.

    scene.hpp
    Scene class

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

#ifndef XTRACER_SCENE_HPP_INCLUDED
#define XTRACER_SCENE_HPP_INCLUDED

#include <string>

#include <nmath/intinfo.h>
#include <nmath/vector.h>
#include <nmath/geometry.h>
#include <ncf/ncf1.hpp>

#include "camera.hpp"
#include "light.hpp"
#include "material.hpp"
#include "texture.hpp"
#include "object.hpp"
#include "photonmap.hpp"

using NMath::Geometry;
using NCF::NCF1;

class Scene
{
	friend class Renderer;
	private:
		Scene(const Scene &);
		Scene &operator =(const Scene &);

	public:
		Scene();
		~Scene();

		const char *name();
		const char *source();

		unsigned int load(const char *filename);
		void apply_modifiers();
		unsigned int build();		// Build the scene data according to the scene tree

		const ColorRGBf &ambient();
		void ambient(const ColorRGBf &ambient);

		unsigned int set_camera(const char *name);

		unsigned int create_light(NCF1 *p);
		unsigned int create_material(NCF1 *p);
		unsigned int create_texture(NCF1 *p);
		unsigned int create_geometry(NCF1 *p);
		unsigned int create_object(NCF1 *p);

		bool intersection(const Ray &ray, IntInfo &info, std::string &obj);

		// The camera
		Camera *camera;

	private:
		// RETURN CODES:
		//  0. Everything went well.
		//  1. The resource was not found.
		unsigned int destroy_camera(const char *name);
		unsigned int destroy_light(const char *name);
		unsigned int destroy_material(const char *name);
		unsigned int destroy_texture(const char *name);
		unsigned int destroy_geometry(const char *name);
		unsigned int destroy_object(const char *name);

		// Maps of the scene entities		
		std::map<std::string, Camera*   > m_cameras;
		std::map<std::string, Light*    > m_lights;
		std::map<std::string, Material* > m_materials;
		std::map<std::string, Texture2D*> m_textures;
		std::map<std::string, Geometry* > m_geometry;
		std::map<std::string, Object*   > m_objects;

		// Ambient
		ColorRGBf m_ambient;	// intensity

		// The scene's source filepath and filename
		const std::string m_source;
		
		// The parser data tree
		NCF1 m_scene;

		// This will cleanup all the allocated memory
		void release();

		PhotonMap m_pm_global;
		PhotonMap m_pm_caustic;
};

#endif /* XTRACER_SCENE_HPP_INCLUDED */
