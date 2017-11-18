#ifndef XTCORE_SCENE_HPP_INCLUDED
#define XTCORE_SCENE_HPP_INCLUDED

#include <string>
#include <vector>
#include <list>

#include <nmath/vector.h>
#include <nmath/intinfo.h>

#include "camera.h"
#include "material.h"
#include "sampler_tex.h"
#include "geometry.h"
#include "object.h"
#include "cubemap.h"

using nimg::ColorRGBf;

typedef std::map<std::string, xtcore::assets::ICamera   *> CamCollection;
typedef std::map<std::string, xtcore::assets::IMaterial *> MatCollection;
typedef std::map<std::string, xtcore::assets::Geometry  *> GeoCollection;
typedef std::map<std::string, xtcore::assets::Object    *> ObjCollection;

struct light_t
{
    xtcore::assets::Geometry  *light;
    xtcore::assets::IMaterial *material;
};

class Scene
{
public:
	Scene(const Scene &);
	Scene &operator =(const Scene &);

    void get_light_sources(std::vector<light_t> &lights);
 	 Scene();
	~Scene();

	void apply_modifiers();
	unsigned int build();

	const ColorRGBf &ambient();
	void ambient(const ColorRGBf &ambient);

	xtcore::assets::ICamera *get_camera(const char *name);

    nimg::ColorRGBf sample_cubemap(const NMath::Vector3f &direction) const;
	bool intersection(const NMath::Ray &ray, NMath::IntInfo &info, std::string &obj);

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
    xtcore::assets::Cubemap *m_cubemap;

	// This will cleanup all the allocated memory
	void release();
};

#endif /* XTCORE_SCENE_HPP_INCLUDED */
