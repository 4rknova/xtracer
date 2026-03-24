#ifndef XTCORE_SCENE_HPP_INCLUDED
#define XTCORE_SCENE_HPP_INCLUDED

#include <vector>
#include <list>
#include <cstddef>
#include <map>

#include <nmath/vector.h>

#include "octree.h"
#include "math/hitrecord.h"
#include "math/surface.h"
#include "strpool.h"
#include "camera.h"
#include "material.h"
#include "medium.h"
#include "sampler/sampler_tex.h"
#include "sampler.h"
#include "object.h"

using nmath::Vector3f;
using nimg::ColorRGBf;

namespace xtcore {

typedef std::map<HASH_UINT64, xtcore::asset::ICamera   *> CamCollection;
typedef std::map<HASH_UINT64, xtcore::asset::IMaterial *> MatCollection;
typedef std::map<HASH_UINT64, xtcore::asset::ISurface  *> GeoCollection;
typedef std::map<HASH_UINT64, xtcore::asset::Object    *> ObjCollection;
typedef std::map<HASH_UINT64, xtcore::asset::medium::IMedium *> MediumCollection;

struct light_t
{
    xtcore::asset::ISurface  *light;
    xtcore::asset::IMaterial *material;
};

struct spatial_index_stats_t
{
    size_t total_objects;
    size_t finite_objects;
    size_t infinite_objects;
    size_t tlas_nodes;
    size_t tlas_leaves;
    double build_ms;
    unsigned long long build_count;
};

class Scene
{
    public:
	Scene(const Scene &);
	Scene &operator =(const Scene &);

    void get_light_sources(std::vector<light_t> &lights);
 	 Scene();
	~Scene();

    float time;

    const xtcore::asset::Object    *get_object   (HASH_ID obj_id);
    const xtcore::asset::IMaterial *get_material (HASH_ID obj_id);
    const xtcore::asset::ISurface  *get_surface  (HASH_ID obj_id);

	void apply_modifiers();
    nmath::scalar_t distance(nmath::Vector3f p, HASH_ID &object) const;

	const ColorRGBf &ambient();
	void ambient(const ColorRGBf &ambient);

	xtcore::asset::ICamera *get_camera(HASH_UINT64 id);
    const xtcore::asset::medium::IMedium *get_object_medium(HASH_UINT64 object_id) const;
    bool has_object_medium(HASH_UINT64 object_id) const;
    void set_object_medium(HASH_UINT64 object_id, xtcore::asset::medium::IMedium *medium);
    void clear_object_medium(HASH_UINT64 object_id);

    nimg::ColorRGBf sample_environment(const Vector3f &direction) const;
	bool intersection(const Ray &ray, hit_record_t &hit_record);
    void rebuild_spatial_index();
    void mark_spatial_index_dirty();

	int destroy_camera   (HASH_UINT64 id);
	int destroy_material (HASH_UINT64 id);
	int destroy_surface  (HASH_UINT64 id);
	int destroy_object   (HASH_UINT64 id);

	// Maps of the scene entities
	CamCollection m_cameras;
	MatCollection m_materials;
	GeoCollection m_surface;
	ObjCollection m_objects;
    MediumCollection m_media;

	// Ambient
	nimg::ColorRGBf m_ambient;	// intensity

	// The scene's source filepath and filename
	std::string m_name;
	std::string m_source;
    std::string m_description;
    std::string m_version;
    std::string m_default_camera;

    xtcore::sampler::ISampler *m_environment;

	// This will cleanup all the allocated memory
	void release();

    private:
    struct tlas_item_t {
        HASH_ID object_id;
        AABB3 aabb;
        Vector3f center;
    };

    struct tlas_node_t {
        AABB3 aabb;
        int left;
        int right;
        size_t first;
        size_t count;
    };

    bool is_finite_aabb(const AABB3 &aabb) const;
    bool intersects_node(const AABB3 &aabb, const Ray &ray) const;
    int build_tlas_node(size_t first, size_t count);
    void update_tlas_node_bounds(tlas_node_t &node);

    bool m_spatial_index_dirty;
    std::vector<tlas_item_t> m_tlas_items;
    std::vector<tlas_node_t> m_tlas_nodes;
    std::vector<HASH_ID> m_infinite_objects;
};

} /* namespace xtcore */

namespace xtcore {
spatial_index_stats_t get_last_spatial_index_stats();
}

#endif /* XTCORE_SCENE_HPP_INCLUDED */
