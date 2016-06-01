#ifndef XTRACER_SCENE_HPP_INCLUDED
#define XTRACER_SCENE_HPP_INCLUDED

#include <string>

#include <nmath/vector.h>
#include <nmath/intinfo.h>
#include <nmath/geometry.h>
#include <ncf/ncf1.hpp>

#include <xtcore/camera.hpp>
#include <xtcore/light.hpp>
#include <xtcore/material.hpp>
#include <xtcore/texture.hpp>
#include <xtcore/object.hpp>

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
};

#endif /* XTRACER_SCENE_HPP_INCLUDED */
