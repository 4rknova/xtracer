#include <algorithm>
#include <vector>
#include <string>
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

    std::map<std::string, xtcore::assets::Object*>::iterator oit = m_objects.begin();
    std::map<std::string, xtcore::assets::Object*>::iterator oet = m_objects.end();

    for(; oit != oet; ++oit) {
        if (!(*oit).second) continue;

        std::map<std::string, xtcore::assets::Geometry* >::iterator git = m_geometry.find((*oit).second->geometry);
        std::map<std::string, xtcore::assets::Geometry* >::iterator get = m_geometry.end();
        std::map<std::string, xtcore::assets::IMaterial*>::iterator mit = m_materials.find((*oit).second->material);
        std::map<std::string, xtcore::assets::IMaterial*>::iterator met = m_materials.end();

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
void purge(std::map<std::string, T*> &map)
{
	if(!map.empty()) {
		for (typename std::map<std::string, T*>::iterator it = map.begin(); it != map.end(); ++it) {
			Log::handle().post_debug("Releasing %s..", (*it).first.c_str());
			delete (*it).second;
            (*it).second = 0;
		}
		map.clear();
	}
}

template<typename T>
int purge(std::map<std::string, T*> &map, const char *name)
{
	typename std::map<std::string, T*>::iterator it = map.find(name);
	if (it == map.end()) return 1;
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

	purge(m_materials);
	purge(m_geometry);
	purge(m_objects);
}

int Scene::destroy_camera   (const char *name) { return purge(m_cameras  , name); }
int Scene::destroy_material (const char *name) { return purge(m_materials, name); }
int Scene::destroy_geometry (const char *name) { return purge(m_geometry , name); }
int Scene::destroy_object   (const char *name) { return purge(m_objects  , name); }

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

xtcore::assets::ICamera *Scene::get_camera(const char *name)
{
    std::map<std::string, xtcore::assets::ICamera *>::iterator et = m_cameras.end();
    std::map<std::string, xtcore::assets::ICamera *>::iterator ft;

    if (!name || strlen(name) == 0) {
        ft = m_cameras.find(XTPROTO_PROP_DEFAULT);
        if (ft != et) return ft->second;
    }
    else {
        ft = m_cameras.find(name);
        if (ft != et) return ft->second;
    }

	return 0;
}

bool Scene::intersection(const NMath::Ray &ray, NMath::IntInfo &info, std::string &obj)
{
	IntInfo test, res;

	std::map<std::string, xtcore::assets::Object *>::iterator it;
	for (it = m_objects.begin(); it != m_objects.end(); ++it) {
		// test all the objects and find the closest intersection
        xtcore::assets::Geometry *geom = m_geometry[((*it).second)->geometry.c_str()];

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
