#include <algorithm>
#include <vector>
#include <fstream>
#include <ncf/util.h>
#include <nmath/mutil.h>
#include <nmath/vector.h>
#include <nmath/geometry.h>
#include <nmath/sphere.h>
#include <nmath/plane.h>
#include <nmath/triangle.h>
#include <nmesh/transform.h>
#include <nmesh/obj.h>
#include <nmesh/structs.h>
#include <nmesh/mesh.h>
#include <nimg/checkerboard.h>
#include "proto.h"
#include "parseutil.h"
#include "log.h"
#include "scene.h"
#include "cam_perspective.h"
#include "cam_ods.h"
#include "cam_erp.h"

using NMath::Vector2f;
using NMath::Vector3f;
using NMath::Geometry;
using NMath::Plane;
using NMath::Triangle;
using NMath::Sphere;
using ncf::util::path_comp;
using ncf::util::trim;
using ncf::util::split;
using ncf::util::to_double;
using ncf::util::to_bool;
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
    : m_cubemap(0)
{}

Scene::~Scene()
{
	release();
}

void Scene::get_light_sources(std::vector<light_t> &lights)
{
    lights.clear();

    std::map<HASH_UINT64, xtcore::assets::Object*>::iterator oit = m_objects.begin();
    std::map<HASH_UINT64, xtcore::assets::Object*>::iterator oet = m_objects.end();

    for(; oit != oet; ++oit) {
        if (!(*oit).second) continue;

        std::map<HASH_UINT64, xtcore::assets::Geometry* >::iterator
            git = m_geometry.find((*oit).second->geometry)
          , get = m_geometry.end();
        std::map<HASH_UINT64, xtcore::assets::IMaterial*>::iterator
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
    if (m_cubemap) {
        delete m_cubemap;
        m_cubemap = 0;
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

nimg::ColorRGBf Scene::sample_cubemap(const NMath::Vector3f &direction) const
{
    return m_cubemap ? m_cubemap->sample(direction) : nimg::ColorRGBf(0,0,0);
}

const ColorRGBf &Scene::ambient()
{
	return m_ambient;
}

void Scene::ambient(const ColorRGBf &ambient)
{
	m_ambient = ambient;
}

xtcore::assets::ICamera *Scene::get_camera(HASH_UINT64 id)
{
    std::map<HASH_UINT64, xtcore::assets::ICamera *>::iterator et = m_cameras.end()
                                                             , ft = m_cameras.find(id);
    if (ft != et) return ft->second;

	return 0;
}

bool Scene::intersection(const NMath::Ray &ray, NMath::IntInfo &info, HASH_UINT64 &obj)
{
	IntInfo test, res;

	std::map<HASH_UINT64, xtcore::assets::Object *>::iterator it;

	for (it = m_objects.begin(); it != m_objects.end(); ++it) {
        xtcore::assets::Geometry *geom = m_geometry[((*it).second)->geometry];

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
}

} /* namespace xtcore */
