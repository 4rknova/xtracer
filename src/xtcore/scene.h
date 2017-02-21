#ifndef XTRACER_SCENE_HPP_INCLUDED
#define XTRACER_SCENE_HPP_INCLUDED

#include <string>
#include <vector>
#include <list>

#include <nmath/vector.h>
#include <nmath/intinfo.h>
#include <ncf/ncf.h>

#include "camera.h"
#include "material.h"
#include "texture.h"
#include "geometry.h"
#include "object.h"

using nimg::ColorRGBf;

using xtracer::assets::Geometry;
using xtracer::assets::ICamera;
using xtracer::assets::Material;
using xtracer::assets::Object;
using xtracer::assets::Texture2D;

struct light_t
{
    Geometry *light;
    Material *material;
};

class Scene
{
	friend class Renderer;
	private:
		Scene(const Scene &);
		Scene &operator =(const Scene &);

        void get_light_sources(std::vector<light_t> &lights);

	public:
		Scene();
		~Scene();

		const char *name();
		const char *source();

		int load(const char *filename, const std::list<std::string> &modifiers);
		void apply_modifiers();
		unsigned int build();

		const ColorRGBf &ambient();
		void ambient(const ColorRGBf &ambient);

		ICamera *get_camera();
        std::string camera;

		bool intersection(const NMath::Ray &ray, NMath::IntInfo &info, std::string &obj);

		void create_camera   (NCF *p);
		void create_material (NCF *p);
		void create_texture  (NCF *p);
		void create_geometry (NCF *p);
		void create_object   (NCF *p);

		int destroy_camera   (const char *name);
		int destroy_material (const char *name);
		int destroy_texture  (const char *name);
		int destroy_geometry (const char *name);
		int destroy_object   (const char *name);

		// Maps of the scene entities
		std::map<std::string, ICamera*  > m_cameras;
		std::map<std::string, Material* > m_materials;
		std::map<std::string, Texture2D*> m_textures;
		std::map<std::string, Geometry* > m_geometry;
		std::map<std::string, Object*   > m_objects;

		// Ambient
		ColorRGBf m_ambient;	// intensity

		// The scene's source filepath and filename
		std::string m_name;
		std::string m_source;

		// This will cleanup all the allocated memory
		void release();
};

#endif /* XTRACER_SCENE_HPP_INCLUDED */
