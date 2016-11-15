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

NMath::Vector2f Scene::deserialize_tex2(const NCF *node, const NMath::Vector2f def)
{
	NMath::Vector2f res = def;

	if (node) {
		const char *u = node->get_property_by_name(XTPROTO_PROP_CRD_U);
		const char *v = node->get_property_by_name(XTPROTO_PROP_CRD_V);

		res = NMath::Vector2f(
			u ? (NMath::scalar_t)to_double(u) : def.x,
			v ? (NMath::scalar_t)to_double(v) : def.y
		);
	}

	return res;
}

ColorRGBf Scene::deserialize_col3(const NCF *node, const ColorRGBf def)
{
	ColorRGBf res = def;

	if (node) {
		const char *r = node->get_property_by_name(XTPROTO_PROP_COL_R);
		const char *g = node->get_property_by_name(XTPROTO_PROP_COL_G);
		const char *b = node->get_property_by_name(XTPROTO_PROP_COL_B);

		res = ColorRGBf(
			r ? (NMath::scalar_t)to_double(r) : def.r(),
			g ? (NMath::scalar_t)to_double(g) : def.g(),
			b ? (NMath::scalar_t)to_double(b) : def.b()
		);
	}

	return res;
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

NMath::Vector3f Scene::deserialize_vec3(const NCF *node, const NMath::Vector3f def)
{
	NMath::Vector3f res = def;

	if (node) {
		const char *x = node->get_property_by_name(XTPROTO_PROP_CRD_X);
		const char *y = node->get_property_by_name(XTPROTO_PROP_CRD_Y);
		const char *z = node->get_property_by_name(XTPROTO_PROP_CRD_Z);

		res = NMath::Vector3f(
			x ? (NMath::scalar_t)to_double(x) : def.x,
			y ? (NMath::scalar_t)to_double(y) : def.y,
			z ? (NMath::scalar_t)to_double(z) : def.z
		);
	}

	return res;
}

NMath::scalar_t Scene::deserialize_numf(const char *val, const NMath::scalar_t def)
{
	return val ? (NMath::scalar_t)to_double(val) : def;
}

bool Scene::deserialize_bool(const char *val, const bool def)
{
	if (!val) return false;
	std::string s = val;
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
	return ( (s == "yes") || (s == "true") ) ? true : false;
}

std::string Scene::deserialize_cstr(const char *val, const char* def)
{
	return val ? val : def;
}

const char *Scene::source()
{
	return m_source.c_str();
}


template<typename T>
void purge(std::map<std::string, T*> &map)
{
	if(!map.empty()) {
		for (typename std::map<std::string, T*>::iterator it = map.begin(); it != map.end(); ++it) {
//			Log::handle().log_message("Releasing %s..", (*it).first.c_str());
			delete (*it).second;
		}
		map.clear();
	}
}

template<typename T>
unsigned int purge(std::map<std::string, T*> &map, const char *name)
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

unsigned int Scene::destroy_camera(const char *name)
{
	return purge(m_cameras, name);
}

unsigned int Scene::destroy_material(const char *name)
{
	return purge(m_materials, name);
}

unsigned int Scene::destroy_texture(const char *name)
{
	return purge(m_textures, name);
}

unsigned int Scene::destroy_geometry(const char *name)
{
	return purge(m_geometry, name);
}

unsigned int Scene::destroy_object(const char *name)
{
	return purge(m_objects, name);
}

unsigned int Scene::load(const char *filename, std::list<std::string> modifiers)
{
    if(!filename) return 1;

	Log::handle().log_message("Loading scene [%s]..", filename);

    NCF root;
    root.set_source(filename);

	if (root.parse())  {
		Log::handle().log_error("Failed to parse the scene script.");
		return 2;
	}

    // Mods are of the form: group.group.property:value
	std::list<std::string>::iterator mod_it = modifiers.begin();
	std::list<std::string>::iterator mod_et = modifiers.end();

    for (; mod_it != mod_et; ++mod_it) {
        std::string mod = (*mod_it);
		if (mod.find_last_of(':') == std::string::npos) {
			Log::handle().log_warning("Invalid rule: %s", mod.c_str());
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

	Log::handle().log_message("Setting up the scene environment..");

	unsigned int count = 0;

	m_source   = deserialize_cstr(filename);
	m_name     = root.get_property_by_name(XTPROTO_PROP_TITLE);
	m_ambient  = deserialize_col3(root.get_group_by_name(XTPROTO_PROP_IAMBN))
			   * deserialize_numf(root.get_property_by_name(XTPROTO_PROP_KAMBN));

	std::list<std::string> sections;
	sections.push_back(XTPROTO_NODE_CAMERA);
	sections.push_back(XTPROTO_NODE_GEOMETRY);
	sections.push_back(XTPROTO_NODE_MATERIAL);
	sections.push_back(XTPROTO_NODE_TEXTURE);
	sections.push_back(XTPROTO_NODE_OBJECT);

	Log::handle().log_message("Building the scene objects..");

	std::list<std::string>::iterator it = sections.begin();
	std::list<std::string>::iterator et = sections.end();

	for (; it != et; ++it) {
		count = root.get_group_by_name((*it).c_str())->count_groups();
		if (count) {
			Log::handle().log_message("Processing section: %s", (*it).c_str());
			for (unsigned int i = 0; i < count; ++i) {
				NCF *lnode = root.get_group_by_name((*it).c_str())->get_group_by_index(i);

				     if (!(*it).compare(XTPROTO_NODE_CAMERA   )) create_camera(lnode);
				else if (!(*it).compare(XTPROTO_NODE_MATERIAL )) create_material(lnode);
				else if (!(*it).compare(XTPROTO_NODE_TEXTURE  )) create_texture(lnode);
				else if (!(*it).compare(XTPROTO_NODE_GEOMETRY )) create_geometry(lnode);
			    else if (!(*it).compare(XTPROTO_NODE_OBJECT   )) create_object(lnode);
			}
		}
	}

	Log::handle().log_message("Scene loaded.");
	return 0;
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

unsigned int Scene::create_camera(NCF *p)
{
    if (!p) return 1;

    std::string type = deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_TYPE));

    ICamera *camera = NULL;

    if (!type.compare(XTPROTO_LTRL_CAM_THINLENS)) {
	    NMath::Vector3f pos     = deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_POSITION));
    	NMath::Vector3f target  = deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_TARGET));
    	NMath::Vector3f up      = deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_UP));
	    NMath::scalar_t fov     = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_FOV));
    	NMath::scalar_t flength = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_FLENGTH));
    	NMath::scalar_t ap      = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_APERTURE));

	    camera = new (std::nothrow) CamPerspective();
        ((CamPerspective *)camera)->position = pos;
        ((CamPerspective *)camera)->target   = target;
        ((CamPerspective *)camera)->up       = up;
        ((CamPerspective *)camera)->fov      = fov;
        ((CamPerspective *)camera)->flength  = flength;
        ((CamPerspective *)camera)->aperture = ap;
    }
    else if (!type.compare(XTPROTO_LTRL_CAM_ODS)) {
	    NMath::Vector3f pos = deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_POSITION));
	    NMath::Vector3f orn = deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_ORIENTATION));
	    NMath::scalar_t ipd = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_IPD));

        camera = new (std::nothrow) CamODS();
        ((CamODS *)camera)->position    = pos;
        ((CamODS *)camera)->orientation = orn;
        ((CamODS *)camera)->ipd         = ipd;
    }
    else if (!type.compare(XTPROTO_LTRL_CAM_ERP)) {
	    NMath::Vector3f pos = deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_POSITION));
	    NMath::Vector3f orn = deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_ORIENTATION));

        camera = new (std::nothrow) CamERP();
        ((CamERP *)camera)->position    = pos;
        ((CamERP *)camera)->orientation = orn;
    }
    else {
		Log::handle().log_warning("Unsupported camera type %s [%s]. Skipping..", p->get_name(), type.c_str());
		return 2;
    }

    // Destroy the old instance (if one exists).
	unsigned int res = destroy_camera(p->get_name());

	// Add it to the list.
	m_cameras[p->get_name()] = camera;

	return res ? 0 : 3;
}

unsigned int Scene::create_geometry(NCF *p)
{
	if (!p) return 1;

	Geometry *geometry = NULL;

	std::string type = deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_TYPE));

	if (!type.compare(XTPROTO_LTRL_PLANE)) {
		geometry = new (std::nothrow) Plane;

		if (!geometry) return 2;

		((Plane *)geometry)->normal   = deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_NORMAL));
		((Plane *)geometry)->distance = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_DISTANCE));
	}
	else if (!type.compare(XTPROTO_LTRL_SPHERE)) {
		geometry = new (std::nothrow) Sphere;

		if (!geometry) return 2;

		((Sphere *)geometry)->origin = deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_POSITION));
		((Sphere *)geometry)->radius = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_RADIUS));
	}
    else if (!type.compare(XTPROTO_LTRL_POINT)) {
        geometry = new (std::nothrow) Sphere;

        if (!geometry) return 2;

		((Sphere *)geometry)->origin = deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_POSITION));
		((Sphere *)geometry)->radius = 0;
    }
	else if (!type.compare(XTPROTO_LTRL_TRIANGLE)) {
		geometry = new (std::nothrow) Triangle;

		if (!geometry) return 2;

		for (unsigned int i = 0; i < 3; ++i) {
			NCF *vnode = p->get_group_by_name(XTPROTO_PROP_VRTXDATA)->get_group_by_index(i);
			((Triangle *)geometry)->v[i]  = deserialize_vec3(vnode);
			((Triangle *)geometry)->tc[i] = deserialize_tex2(vnode);
		}
	}
	// - Mesh
	else if (!type.compare(XTPROTO_LTRL_MESH)) {
		geometry = new (std::nothrow) NMesh::Mesh;

		// Data source
		std::string f = p->get_property_by_name(XTPROTO_PROP_SOURCE);

		// Open source file from relative path
		std::string base, file;
		path_comp(m_source, base, file);
		base.append(f);

		Log::handle().log_message("Loading data from %s", base.c_str());

        NMesh::object_t obj;

		if (NMesh::IO::Import::obj(base.c_str(), obj))
		{
			Log::handle().log_warning("Failed to load mesh from %s", f.c_str());
			delete geometry;
			return 1;
		}

		if (p->query_group(XTPROTO_PROP_ROTATION)) {
			NMath::Vector3f v = deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_ROTATION));
			NMesh::Mutator::rotate(obj, v.x, v.y, v.z);
		}

		if (p->query_group(XTPROTO_PROP_SCALE)) {
			NMath::Vector3f v = deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_SCALE));
			NMesh::Mutator::scale(obj, v.x, v.y, v.z);
		}

		if (p->query_group(XTPROTO_PROP_TRANSLATION)) {
			NMath::Vector3f v = deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_TRANSLATION));
			NMesh::Mutator::translate(obj, v.x, v.y, v.z);
		}

		Log::handle().log_message("Building octree..");
		((NMesh::Mesh *)geometry)->build_octree(obj);
	}
	// unknown
	else {
		Log::handle().log_warning("Unsupported geometry type %s [%s]. Skipping..", p->get_name(), type.c_str());
		return 1;
	}

	geometry->uv_scale = Vector2f(
		deserialize_numf(p->get_property_by_name(XTPROTO_PROP_USCALE)),
	    deserialize_numf(p->get_property_by_name(XTPROTO_PROP_VSCALE))
	);

	geometry->calc_aabb();

	// Destroy the old instance (if one exists).
	unsigned int res = destroy_geometry(p->get_name());

	// Add it to the list.
	m_geometry[p->get_name()] = geometry;

	return res ? 0 : 3;
}

unsigned int Scene::create_material(NCF *p)
{
	if (!p) return 1;

	Material *material = new (std::nothrow) Material;

	if (!material) return 2;

	std::string type = deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_TYPE));

	     if (!type.compare(XTPROTO_LTRL_LAMBERT)   ) material->type = MATERIAL_LAMBERT;
	else if (!type.compare(XTPROTO_LTRL_PHONG)     ) material->type = MATERIAL_PHONG;
	else if (!type.compare(XTPROTO_LTRL_BLINNPHONG)) material->type = MATERIAL_BLINNPHONG;
	else if (!type.compare(XTPROTO_LTRL_EMISSIVE  )) material->type = MATERIAL_EMISSIVE;
	else {
		Log::handle().log_warning("Unsupported material %s. Skipping..", p->get_name());
		delete material;
		return 1;
	}

	material->ambient      = deserialize_col3(p->get_group_by_name(XTPROTO_PROP_IAMBN));
	material->specular     = deserialize_col3(p->get_group_by_name(XTPROTO_PROP_ISPEC));
	material->diffuse      = deserialize_col3(p->get_group_by_name(XTPROTO_PROP_IDIFF));
	material->emissive     = deserialize_col3(p->get_group_by_name(XTPROTO_PROP_EMISSIVE));
	material->kdiff        = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_KDIFF), 1.f);
	material->kspec        = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_KSPEC), 0.f);
	material->ksexp        = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_KEXPN), 0.f);
	material->roughness    = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_ROUGH), 0.f);
	material->transparency = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_TRSPC), 0.f);
	material->reflectance  = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_REFLC), 0.f);
	material->ior          = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_IOR  ), 1.f);

	unsigned int res = destroy_material(p->get_name());
	m_materials[p->get_name()] = material;

	return res ? 0 : 3;
}

unsigned int Scene::create_texture(NCF *p)
{
	if (!p)	return 1;

	Texture2D *texture = new (std::nothrow) Texture2D;

	if (!texture) return 2;

	std::string script_base, script_filename;
	path_comp(m_source, script_base, script_filename);

    std::string fname = deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_SOURCE));
	std::string source = script_base + fname;

	Log::handle().log_message("Loading data from %s", source.c_str());
	if (texture->load(source.c_str())) {
		Log::handle().log_warning("Failed to load texture [%s->%s]", p->get_name(), source.c_str());
		Log::handle().log_warning("Replacing with checkerboard..");

		Pixmap tex;
		ColorRGBAf a   = ColorRGBAf(0.5,0.5,0.5,1);
		ColorRGBAf b   = ColorRGBAf(1,1,1,1);
		nimg::generator::checkerboard(tex, 2, 2, a, b);

		texture->load(tex);
	}

	// Destroy the old texture from the list if it exists.
	unsigned int res = destroy_texture(p->get_name());

	// Add the texture to the list.
	m_textures[p->get_name()] = texture;

	return res ? 0 : 3;

}

unsigned int Scene::create_object(NCF *p)
{
	if (!p)	return 1;

	Object *object = new (std::nothrow) Object;

	{
		const char *n = p->get_property_by_name(XTPROTO_PROP_OBJ_GEO);
		if (m_geometry.find(n) != m_geometry.end()) {
			object->geometry = deserialize_cstr(n);
		}
		else {
			Log::handle().log_warning("At object: %s, geometry %s does not exist.", p->get_name(), n);
			delete object;
			return 5;
		}
	}

	{
		const char *n = p->get_property_by_name(XTPROTO_PROP_OBJ_MAT);
		if (m_materials.find(n) != m_materials.end()) object->material = deserialize_cstr(n);
		else {
			Log::handle().log_warning("At object: %s, material %s does not exist.", p->get_name(), n);
			delete object;
			return 5;
		}
	}

	{
		const char *n = p->get_property_by_name(XTPROTO_PROP_OBJ_TEX);

		if (n) {
			if (m_textures.find(n) != m_textures.end()) object->texture = deserialize_cstr(n);
			else {
				Log::handle().log_warning("At object: %s, texture %s does not exist.", p->get_name(), n);
				delete object;
				return 5;
			}
		}
	}

	// Destroy the old instance (if one exists).
	unsigned int res = destroy_object(p->get_name());

	// Add the object to the list
	m_objects[p->get_name()] = object;

	return res ? 0 : 3;
}

bool Scene::intersection(const NMath::Ray &ray, NMath::IntInfo &info, std::string &obj)
{
	IntInfo test, res;

	std::map<std::string, Object *>::iterator it;
	for (it = m_objects.begin(); it != m_objects.end(); it++) {
		// test all the objects and find the closest intersection
		if((m_geometry[(*it).second->geometry.c_str()])->intersection(ray, &test)) {
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
