#include <algorithm>
#include <vector>
#include <fstream>
#include <ncf/util.h>
#include <nmath/mutil.h>
#include <nmath/vector.h>
#include <nmesh/transform.h>
#include <nmesh/obj.h>
#include <nmesh/structs.h>
#include <nimg/checkerboard.h>
#include "math/surface.h"
#include "math/sphere.h"
#include "math/plane.h"
#include "math/triangle.h"
#include "mesh.h"
#include "proto.h"
#include "parseutil.h"
#include "log.h"
#include "scene.h"

using NMath::Vector2f;
using NMath::Vector3f;
using xtcore::Log;

namespace xtcore {

// Private constructor & assignment operator.
// They are not implemented but declared private
// for the time being.
Scene::Scene(const Scene &)
{}

Scene &Scene::operator =(const Scene &)
{
	return *this;
}

Scene::Scene()
    : m_environment(0)
{}

Scene::~Scene()
{
	release();
}

const xtcore::asset::Object *Scene::get_object(HASH_ID obj_id)
{
    auto it = m_objects.find(obj_id);
    if (it == m_objects.end()) return 0;
    return (*it).second;
}

const xtcore::asset::IMaterial *Scene::get_material(HASH_ID obj_id)
{
    const xtcore::asset::Object *obj = get_object(obj_id);
    auto it = m_materials.find(obj->material);
    if (it == m_materials.end()) return 0;
    return (*it).second;
}

const xtcore::asset::ISurface *Scene::get_surface(HASH_ID obj_id)
{
    const xtcore::asset::Object *obj = get_object(obj_id);
    auto it = m_surface.find(obj->surface);
    if (it == m_surface.end()) return 0;
    return (*it).second;
}

void Scene::get_light_sources(std::vector<light_t> &lights)
{
    lights.clear();

    std::map<HASH_UINT64, xtcore::asset::Object*>::iterator oit = m_objects.begin();
    std::map<HASH_UINT64, xtcore::asset::Object*>::iterator oet = m_objects.end();

    for(; oit != oet; ++oit) {
        if (!(*oit).second) continue;

        std::map<HASH_UINT64, xtcore::asset::ISurface* >::iterator
            git = m_surface.find((*oit).second->surface)
          , get = m_surface.end();
        std::map<HASH_UINT64, xtcore::asset::IMaterial*>::iterator
            mit = m_materials.find((*oit).second->material)
          , met = m_materials.end();

        if (git != get && mit != met) {
            if (!(*mit).second->is_emissive()) continue;
            light_t light;
            light.light    = (*git).second;
            light.material = (*mit).second;
            lights.push_back(light);
        }
    }
}

template<typename T>
void purge(std::map<HASH_UINT64, T*> &map)
{
	if(!map.empty()) {
		for (typename std::map<HASH_UINT64, T*>::iterator it = map.begin(); it != map.end(); ++it) {
			Log::handle().post_debug("Releasing %s..", xtcore::pool::str::get((*it).first));
            xtcore::pool::str::del((*it).first);
			delete (*it).second;
            (*it).second = 0;
		}
		map.clear();
	}
}

template<typename T>
int purge(std::map<HASH_UINT64, T*> &map, HASH_UINT64 id)
{
	typename std::map<HASH_UINT64, T*>::iterator it = map.find(id);
	if (it == map.end()) return 1;
	Log::handle().post_debug("Releasing %s..", xtcore::pool::str::get((*it).first));
    xtcore::pool::str::del((*it).first);
	delete (*it).second;
	map.erase(it);
	return 0;
}

void Scene::release()
{
    if (m_environment) {
        delete m_environment;
        m_environment = 0;
    }

    purge(m_cameras);
	purge(m_materials);
	purge(m_surface);
	purge(m_objects);
}

int Scene::destroy_camera   (HASH_UINT64 id) { return purge(m_cameras  , id); }
int Scene::destroy_material (HASH_UINT64 id) { return purge(m_materials, id); }
int Scene::destroy_surface (HASH_UINT64 id) { return purge(m_surface , id); }
int Scene::destroy_object   (HASH_UINT64 id) { return purge(m_objects  , id); }

nimg::ColorRGBf Scene::sample_environment(const Vector3f &direction) const
{
    return m_environment ? m_environment->sample(direction) : nimg::ColorRGBf(0,0,0);
}

const ColorRGBf &Scene::ambient()
{
	return m_ambient;
}

void Scene::ambient(const ColorRGBf &ambient)
{
	m_ambient = ambient;
}

xtcore::asset::ICamera *Scene::get_camera(HASH_UINT64 id)
{
    auto et = m_cameras.end()
       , ft = m_cameras.find(id);

    if (ft != et) return ft->second;

	return 0;
}

NMath::scalar_t Scene::distance(NMath::Vector3f p, HASH_ID &object) const
{
    auto it = m_objects.begin();
    auto et = m_objects.end();

    NMath::scalar_t dist = INFINITY;

    for (; it != et; ++it) {
        HASH_ID obj_id = (*it).first;
        HASH_ID geo_id = (*it).second->surface;
        auto geo = m_surface.find(geo_id);
        if (geo == m_surface.end()) continue;

        asset::ISurface *s = (*geo).second;

        NMath::scalar_t temp_dist = s->distance(p);
        if (temp_dist < dist) {
            dist = temp_dist;
            object = obj_id;
        }
    }
    return dist;
}

bool Scene::intersection(const Ray &ray, HitRecord &info)
{
	HitRecord test, res;

    auto it = m_objects.begin()
       , et = m_objects.end();

    bool hit = false;

	for (; it != et; ++it) {
        asset::ISurface *geom = m_surface[((*it).second)->surface];

		if (geom && geom->intersection(ray, &test)) {
            hit = true;

			if (res.t > test.t) {
				res = test;
				res.id_object = (*it).first;
			}
		}
	}
	// copy result to info
	info = res;

	return hit ? true : false;
}

} /* namespace xtcore */
