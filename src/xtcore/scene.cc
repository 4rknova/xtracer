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
using Util::String::path_comp;
using Util::String::trim;
using Util::String::split;
using Util::String::to_double;
using Util::String::to_bool;

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
{}

Scene::~Scene()
{
	release();
}

const char *Scene::source()
{
	return m_source.c_str();
}

void Scene::get_light_sources(std::vector<light_t> &lights)
{
    lights.clear();

    std::map<std::string, Object*>::iterator oit = m_objects.begin();
    std::map<std::string, Object*>::iterator oet = m_objects.end();

    for(; oit != oet; ++oit) {
        if (!(*oit).second) continue;

        std::map<std::string, Geometry*>::iterator git = m_geometry.find((*oit).second->geometry);
        std::map<std::string, Geometry*>::iterator get = m_geometry.end();
        std::map<std::string, Material*>::iterator mit = m_materials.find((*oit).second->material);
        std::map<std::string, Material*>::iterator met = m_materials.end();

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
	purge(m_materials);
	purge(m_textures);
	purge(m_geometry);
	purge(m_objects);
}

int Scene::destroy_camera   (const char *name) { return purge(m_cameras  , name); }
int Scene::destroy_material (const char *name) { return purge(m_materials, name); }
int Scene::destroy_texture  (const char *name) { return purge(m_textures , name); }
int Scene::destroy_geometry (const char *name) { return purge(m_geometry , name); }
int Scene::destroy_object   (const char *name) { return purge(m_objects  , name); }

int Scene::load(const char *filename, const std::list<std::string> &modifiers)
{
    if(!filename) return 1;

	Log::handle().post_message("Loading scene [%s]..", filename);

    NCF root;
    root.set_source(filename);

	if (root.parse())  {
		Log::handle().post_error("Failed to parse the scene script.");
		return 2;
	}

    // Mods are of the form: group.group.property:value
	std::list<std::string>::const_iterator mod_it = modifiers.begin();
	std::list<std::string>::const_iterator mod_et = modifiers.end();

    for (; mod_it != mod_et; ++mod_it) {
        std::string mod = (*mod_it);
		if (mod.find_last_of(':') == std::string::npos) {
			Log::handle().post_warning("Invalid rule: %s", mod.c_str());
			continue;
		}

		NCF *node = &root;
		std::string nleft, nright;
        while((mod.find_first_of('.') != std::string::npos) && (mod.find_first_of(':') > mod.find_first_of('.'))) {
			Util::String::split(mod, nleft, nright, '.');
			mod = nright;
			node = node->get_group_by_name(nleft.c_str());
		}

		Util::String::split(mod, nleft, nright, ':');
		node->set_property(nleft.c_str(), nright.c_str());
	}

	size_t count = 0;

	m_source   = xtracer::io::deserialize_cstr(filename);
	m_name     = root.get_property_by_name(XTPROTO_PROP_TITLE);
	m_ambient  = xtracer::io::deserialize_col3(root.get_group_by_name(XTPROTO_PROP_IAMBN))
			   * xtracer::io::deserialize_numf(root.get_property_by_name(XTPROTO_PROP_KAMBN));

	std::list<std::string> sections;
	sections.push_back(XTPROTO_NODE_CAMERA);
	sections.push_back(XTPROTO_NODE_GEOMETRY);
	sections.push_back(XTPROTO_NODE_MATERIAL);
	sections.push_back(XTPROTO_NODE_TEXTURE);
	sections.push_back(XTPROTO_NODE_OBJECT);

	Log::handle().post_message("Building the scene..");

	std::list<std::string>::iterator it = sections.begin();
	std::list<std::string>::iterator et = sections.end();

	for (; it != et; ++it) {
		count = root.get_group_by_name((*it).c_str())->count_groups();
		if (count) {
			Log::handle().post_message("Processing section: %s", (*it).c_str());
			for (size_t i = 0; i < count; ++i) {
				NCF *lnode = root.get_group_by_name((*it).c_str())->get_group_by_index(i);

				     if (!(*it).compare(XTPROTO_NODE_CAMERA   )) create_camera   (lnode);
				else if (!(*it).compare(XTPROTO_NODE_MATERIAL )) create_material (lnode);
				else if (!(*it).compare(XTPROTO_NODE_TEXTURE  )) create_texture  (lnode);
				else if (!(*it).compare(XTPROTO_NODE_GEOMETRY )) create_geometry (lnode);
			    else if (!(*it).compare(XTPROTO_NODE_OBJECT   )) create_object   (lnode);
			}
		}
	}

	Log::handle().post_message("Scene loaded.");
	return 0;
}


void Scene::create_camera(NCF *p)
{
    Log::handle().post_debug("Loading: %s", p->get_name());
    const char * name = p->get_name();
    xtracer::assets::ICamera *data = xtracer::io::deserialize_camera(m_source.c_str(), p);
    if (!data) {
        Log::handle().post_warning("Failed to load: %s", p->get_name());
        return;
    }
    destroy_camera(name);
    m_cameras[name] = data;
}

void Scene::create_material(NCF *p)
{
    Log::handle().post_debug("Loading: %s", p->get_name());
    const char * name = p->get_name();
    xtracer::assets::Material *data = xtracer::io::deserialize_material(m_source.c_str(), p);
    if (!data) {
        Log::handle().post_warning("Failed to load: %s", p->get_name());
        return;
    }
    destroy_material(name);
    m_materials[name] = data;
}

void Scene::create_texture(NCF *p)
{
    Log::handle().post_debug("Loading: %s", p->get_name());
    const char * name = p->get_name();
    xtracer::assets::Texture2D *data = xtracer::io::deserialize_texture(m_source.c_str(), p);
    if (!data) {
        Log::handle().post_warning("Failed to load: %s", p->get_name());
        return;
    }
    destroy_texture(name);
    m_textures[name] = data;
}

void Scene::create_geometry(NCF *p)
{
    Log::handle().post_debug("Loading: %s", p->get_name());
    const char * name = p->get_name();
    xtracer::assets::Geometry *data = xtracer::io::deserialize_geometry(m_source.c_str(), p);
    if (!data) {
        Log::handle().post_warning("Failed to load: %s", p->get_name());
        return;
    }
    destroy_geometry(name);
    m_geometry[name] = data;
}

void Scene::create_object(NCF *p)
{
    Log::handle().post_debug("Loading: %s", p->get_name());
    const char * name = p->get_name();
    xtracer::assets::Object *data = xtracer::io::deserialize_object(m_source.c_str(), p);
    if (!data) {
        Log::handle().post_warning("Failed to load: %s", p->get_name());
        return;
    }
    destroy_object(name);
    m_objects[name] = data;
}

const ColorRGBf &Scene::ambient()
{
	return m_ambient;
}

void Scene::ambient(const ColorRGBf &ambient)
{
	m_ambient = ambient;
}

ICamera *Scene::get_camera()
{
    std::map<std::string, ICamera *>::iterator et = m_cameras.end();
    std::map<std::string, ICamera *>::iterator ft;

    if (camera.empty()) {
        ft = m_cameras.find(XTPROTO_PROP_DEFAULT);
        if (ft != et) return ft->second;
    }
    else {
        ft = m_cameras.find(camera);
        if (ft != et) return ft->second;
    }

	return 0;
}


bool Scene::intersection(const NMath::Ray &ray, NMath::IntInfo &info, std::string &obj)
{
	IntInfo test, res;

	std::map<std::string, Object *>::iterator it;
	for (it = m_objects.begin(); it != m_objects.end(); it++) {
		// test all the objects and find the closest intersection
        xtracer::assets::Geometry *geom = m_geometry[((*it).second)->geometry.c_str()];

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
