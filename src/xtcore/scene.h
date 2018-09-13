#ifndef XTCORE_SCENE_HPP_INCLUDED
#define XTCORE_SCENE_HPP_INCLUDED

#include <vector>
#include <list>

#include <nmath/vector.h>

#include "octree.h"
#include "math/hitrecord.h"
#include "math/surface.h"
#include "strpool.h"
#include "camera.h"
#include "material.h"
#include "sampler_tex.h"
#include "sampler.h"
#include "object.h"

using NMath::Vector3f;
using nimg::ColorRGBf;

namespace xtcore {

typedef std::map<HASH_UINT64, xtcore::asset::ICamera   *> CamCollection;
typedef std::map<HASH_UINT64, xtcore::asset::IMaterial *> MatCollection;
typedef std::map<HASH_UINT64, xtcore::asset::ISurface  *> GeoCollection;
typedef std::map<HASH_UINT64, xtcore::asset::Object    *> ObjCollection;

struct light_t
{
    xtcore::asset::ISurface  *light;
    xtcore::asset::IMaterial *material;
};

class Scene
{
    public:
	Scene(const Scene &);
	Scene &operator =(const Scene &);

    void get_light_sources(std::vector<light_t> &lights);
 	 Scene();
	~Scene();

    const xtcore::asset::Object    *get_object   (HASH_ID obj_id);
    const xtcore::asset::IMaterial *get_material (HASH_ID obj_id);
    const xtcore::asset::ISurface  *get_surface  (HASH_ID obj_id);

	void apply_modifiers();
    NMath::scalar_t distance(NMath::Vector3f p, HASH_ID &object) const;

	const ColorRGBf &ambient();
	void ambient(const ColorRGBf &ambient);

	xtcore::asset::ICamera *get_camera(HASH_UINT64 id);

    nimg::ColorRGBf sample_environment(const Vector3f &direction) const;
	bool intersection(const Ray &ray, hit_record_t &hit_record);

	int destroy_camera   (HASH_UINT64 id);
	int destroy_material (HASH_UINT64 id);
	int destroy_surface  (HASH_UINT64 id);
	int destroy_object   (HASH_UINT64 id);

	// Maps of the scene entities
	CamCollection m_cameras;
	MatCollection m_materials;
	GeoCollection m_surface;
	ObjCollection m_objects;

	// Ambient
	nimg::ColorRGBf m_ambient;	// intensity

	// The scene's source filepath and filename
	std::string m_name;
	std::string m_source;
    std::string m_description;
    std::string m_version;

    xtcore::sampler::ISampler *m_environment;

	// This will cleanup all the allocated memory
	void release();
};

} /* namespace xtcore */

#endif /* XTCORE_SCENE_HPP_INCLUDED */
