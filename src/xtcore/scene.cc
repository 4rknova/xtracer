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

void Scene::get_light_sources(std::vector<light_t> &lights)
{
    lights.clear();

    std::map<HASH_UINT64, xtcore::asset::Object*>::iterator oit = m_objects.begin();
    std::map<HASH_UINT64, xtcore::asset::Object*>::iterator oet = m_objects.end();

    for(; oit != oet; ++oit) {
        if (!(*oit).second) continue;

        std::map<HASH_UINT64, xtcore::asset::ISurface* >::iterator
            git = m_geometry.find((*oit).second->geometry)
          , get = m_geometry.end();
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
	purge(m_geometry);
	purge(m_objects);
}

int Scene::destroy_camera   (HASH_UINT64 id) { return purge(m_cameras  , id); }
int Scene::destroy_material (HASH_UINT64 id) { return purge(m_materials, id); }
int Scene::destroy_geometry (HASH_UINT64 id) { return purge(m_geometry , id); }
int Scene::destroy_object   (HASH_UINT64 id) { return purge(m_objects  , id); }

nimg::ColorRGBf Scene::sample_cubemap(const Vector3f &direction) const
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


void Scene::build()
{
    static_geometry.clear();

    auto it = m_objects.begin();
    auto et = m_objects.end();

    for (; it != et; ++it) {
        geonode_t n;
        n.id   = (*it).first;
        n.data = m_geometry[(*it).second->geometry];
        n.data->calc_aabb();
        static_geometry.add(n.data->aabb, n);
    }

    static_geometry.max_depth(3);
    static_geometry.build();
}

//#define USE_SCENE_OCTREE

bool Scene::intersection(const Ray &ray, HitRecord &info, HASH_UINT64 &obj)
{
	HitRecord test, res;

	std::map<HASH_UINT64, xtcore::asset::Object *>::iterator it;

#ifndef USE_SCENE_OCTREE
	for (it = m_objects.begin(); it != m_objects.end(); ++it) {
        asset::ISurface *geom = m_geometry[((*it).second)->geometry];

		if (geom && geom->intersection(ray, &test)) {
			if(res.t > test.t) {
				obj = (*it).first;
				memcpy(&res, &test, sizeof(res));
			}
		}
	}
	// copy result to info
	memcpy(&info, &res, sizeof(info));

	return info.t != INFINITY ? true : false;
#else
    bool hit = static_geometry.intersection(ray, &info) != NULL;
    obj = info.id;
    return hit && (info.t != INFINITY);
#endif
}

} /* namespace xtcore */
