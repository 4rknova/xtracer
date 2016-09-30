#ifndef XTRACER_SCENE_HPP_INCLUDED
#define XTRACER_SCENE_HPP_INCLUDED

#include <string>

#include <nmath/vector.h>
#include <nmath/intinfo.h>
#include <nmath/geometry.h>
#include <ncf/ncf.h>

#include <xtcore/camera.h>
#include <xtcore/light.hpp>
#include <xtcore/material.hpp>
#include <xtcore/texture.hpp>
#include <xtcore/object.hpp>

using nimg::ColorRGBf;

#define VALUE_DEFAULT_NUMF (0)
#define VALUE_DEFAULT_TEX2 (NMath::Vector2f(0.f,0.f))
#define VALUE_DEFAULT_VEC3 (NMath::Vector3f(0.f,0.f,0.f))
#define VALUE_DEFAULT_COL3 (ColorRGBf(0.f,0.f,0.f))
#define VALUE_DEFAULT_CSTR ("")
#define VALUE_DEFAULT_BOOL false

using NMath::Geometry;
using xtracer::assets::ICamera;

class Scene
{
	friend class Renderer;
	private:
		Scene(const Scene &);
		Scene &operator =(const Scene &);

		ColorRGBf deserialize_col3(const NCF *node, const ColorRGBf def = VALUE_DEFAULT_COL3);
		NMath::Vector2f deserialize_tex2(const NCF *node, const NMath::Vector2f def = VALUE_DEFAULT_TEX2);
		NMath::Vector3f deserialize_vec3(const NCF *node, const NMath::Vector3f def = VALUE_DEFAULT_VEC3);
		NMath::scalar_t deserialize_numf(const char *val, const NMath::scalar_t def = VALUE_DEFAULT_NUMF);
		std::string     deserialize_cstr(const char *val, const char*           def = VALUE_DEFAULT_CSTR);
		bool            deserialize_bool(const char *val, const bool            def = VALUE_DEFAULT_BOOL);

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

		ICamera *get_camera();
        std::string camera;

		unsigned int create_camera(NCF *p);
		unsigned int create_light(NCF *p);
		unsigned int create_material(NCF *p);
		unsigned int create_texture(NCF *p);
		unsigned int create_geometry(NCF *p);
		unsigned int create_object(NCF *p);

		bool intersection(const Ray &ray, IntInfo &info, std::string &obj);

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
		std::map<std::string, ICamera*  > m_cameras;
		std::map<std::string, ILight*   > m_lights;
		std::map<std::string, Material* > m_materials;
		std::map<std::string, Texture2D*> m_textures;
		std::map<std::string, Geometry* > m_geometry;
		std::map<std::string, Object*   > m_objects;

		// Ambient
		ColorRGBf m_ambient;	// intensity

		// The scene's source filepath and filename
		const std::string m_source;

		// The parser data tree
		NCF m_scene;

		// This will cleanup all the allocated memory
		void release();
};

#endif /* XTRACER_SCENE_HPP_INCLUDED */
