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
#include "cubemap.h"

using nimg::ColorRGBf;

typedef std::map<std::string, xtracer::assets::ICamera   *> CamCollection;
typedef std::map<std::string, xtracer::assets::IMaterial *> MatCollection;
typedef std::map<std::string, xtracer::assets::Geometry  *> GeoCollection;
typedef std::map<std::string, xtracer::assets::Object    *> ObjCollection;

struct light_t
{
    xtracer::assets::Geometry  *light;
    xtracer::assets::IMaterial *material;
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

		int load(const char *filename, const std::list<std::string> *modifiers);
		void apply_modifiers();
		unsigned int build();

		const ColorRGBf &ambient();
		void ambient(const ColorRGBf &ambient);

		xtracer::assets::ICamera *get_camera();
        std::string camera;

        nimg::ColorRGBf sample_cubemap(const NMath::Vector3f &direction) const;
		bool intersection(const NMath::Ray &ray, NMath::IntInfo &info, std::string &obj);

		int create_cubemap  (ncf::NCF *p);
		int create_camera   (ncf::NCF *p);
		int create_material (ncf::NCF *p);
		int create_geometry (ncf::NCF *p);
		int create_object   (ncf::NCF *p);

		int destroy_camera   (const char *name);
		int destroy_material (const char *name);
		int destroy_geometry (const char *name);
		int destroy_object   (const char *name);

		// Maps of the scene entities
	    CamCollection m_cameras;
		MatCollection m_materials;
		GeoCollection m_geometry;
		ObjCollection m_objects;

		// Ambient
		nimg::ColorRGBf m_ambient;	// intensity

		// The scene's source filepath and filename
		std::string m_name;
		std::string m_source;
        xtracer::assets::Cubemap *m_cubemap;

		// This will cleanup all the allocated memory
		void release();
};

#endif /* XTRACER_SCENE_HPP_INCLUDED */
