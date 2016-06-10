#include <vector>
#include <string>
#include <fstream>
#include <nmath/mutil.h>
#include <nmath/vector.h>
#include <ncf/util.h>
#include <nmesh/transform.hpp>
#include <nmesh/calcnormals.hpp>
#include <nmesh/obj.hpp>

#include <nmath/geometry.h>
#include <nmath/sphere.h>
#include <nmath/plane.h>
#include <nmath/triangle.h>
#include <nmesh/mesh.hpp>
#include "proto.h"
#include "log.hpp"
#include "scene.h"

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
	: camera(new Camera())
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
			u ? (scalar_t)to_double(u) : def.x,
			v ? (scalar_t)to_double(v) : def.y
		);
	}

	return res;
}

NImg::ColorRGBf Scene::deserialize_col3(const NCF *node, const NImg::ColorRGBf def)
{
	NImg::ColorRGBf res = def;

	if (node) {
		const char *r = node->get_property_by_name(XTPROTO_PROP_COL_R);
		const char *g = node->get_property_by_name(XTPROTO_PROP_COL_G);
		const char *b = node->get_property_by_name(XTPROTO_PROP_COL_B);

		res = NImg::ColorRGBf(
			r ? (scalar_t)to_double(r) : def.r(),
			g ? (scalar_t)to_double(g) : def.g(),
			b ? (scalar_t)to_double(b) : def.b()
		);
	}

	return res;
}

NMath::Vector3f Scene::deserialize_vec3(const NCF *node, const NMath::Vector3f def)
{
	NMath::Vector3f res = def;

	if (node) {
		const char *x = node->get_property_by_name(XTPROTO_PROP_CRD_X);
		const char *y = node->get_property_by_name(XTPROTO_PROP_CRD_Y);
		const char *z = node->get_property_by_name(XTPROTO_PROP_CRD_Z);

		res = NMath::Vector3f(
			x ? (scalar_t)to_double(x) : def.x,
			y ? (scalar_t)to_double(y) : def.y,
			z ? (scalar_t)to_double(z) : def.z
		);
	}

	return res;
}

NMath::scalar_t Scene::deserialize_numf(const char *val, const NMath::scalar_t def)
{
	return val ? (scalar_t)to_double(val) : def;
}

const char *Scene::name()
{
	return m_scene.get_property_by_name(XTPROTO_PROP_TITLE);
}

const char *Scene::source()
{
	return m_source.c_str();
}

void Scene::release()
{
	Log::handle().log_message("Cleaning up..");

	// Release the lights
	if(!m_lights.empty()) {
		Log::handle().log_message("Releasing the lights..");
		for (std::map<std::string, Light *>::iterator it = m_lights.begin(); it != m_lights.end(); ++it) {
			delete (*it).second;
		}
		m_lights.clear();
	}

	// Release the materials
	if(!m_materials.empty()) {
		Log::handle().log_message("Releasing the materials..");
		for (std::map<std::string, Material *>::iterator it = m_materials.begin(); it != m_materials.end(); ++it) {
			delete (*it).second;
		}
		m_materials.clear();
	}

	// Release the textures
	if(!m_textures.empty()) {
		Log::handle().log_message("Releasing the textures..");
		for (std::map<std::string, Texture2D *>::iterator it = m_textures.begin(); it != m_textures.end(); ++it) {
			delete (*it).second;
		}
		m_textures.clear();
	}

	// Release the geometry
	if(!m_geometry.empty()) {
		Log::handle().log_message("Releasing the geometry..");
		for (std::map<std::string, Geometry *>::iterator it = m_geometry.begin(); it != m_geometry.end(); ++it) {
			delete (*it).second;
		}
		m_geometry.clear();
	}

	// Release the objects
	if(!m_objects.empty()) {
		Log::handle().log_message("Releasing the objects..");
		for (std::map<std::string, Object *>::iterator it = m_objects.begin(); it != m_objects.end(); ++it) {
			delete (*it).second;
		}
		m_objects.clear();
	}

	// Release the camera
	Log::handle().log_message("Releasing the camera..");
	delete camera;
	camera = NULL;
}

unsigned int Scene::destroy_camera(const char *name)
{
	std::map<std::string, Camera *>::iterator it = m_cameras.find(name);

	if (it == m_cameras.end())
	    return 1;

	delete (*it).second;
	m_cameras.erase(it);

	return 0;
}

unsigned int Scene::destroy_light(const char *name)
{
	std::map<std::string, Light *>::iterator it = m_lights.find(name);

	if (it == m_lights.end())
	    return 1;

	delete (*it).second;
	m_lights.erase(it);

	return 0;
}

unsigned int Scene::destroy_material(const char *name)
{
	std::map<std::string, Material *>::iterator it = m_materials.find(name);

	if (it == m_materials.end())
	    return 1;

	delete (*it).second;
	m_materials.erase(it);

	return 0;
}

unsigned int Scene::destroy_texture(const char *name)
{
	std::map<std::string, Texture2D *>::iterator it = m_textures.find(name);

	if (it == m_textures.end())
	    return 1;

	delete (*it).second;
	m_textures.erase(it);

	return 0;
}

unsigned int Scene::destroy_geometry(const char *name)
{
	std::map<std::string, Geometry *>::iterator it = m_geometry.find(name);

	if (it == m_geometry.end())
	    return 1;

	delete (*it).second;
	m_geometry.erase(it);

	return 0;
}

unsigned int Scene::destroy_object(const char *name)
{
	std::map<std::string, Object *>::iterator it = m_objects.find(name);

	if (it == m_objects.end())
	    return 1;

	delete (*it).second;
	m_objects.erase(it);

	return 0;
}

unsigned int Scene::load(const char *filename)
{
	Log::handle().log_message("Loading scene [%s]..", filename);
	m_scene.set_source(filename);

	if (m_scene.parse())  {
		Log::handle().log_error("Failed to parse the scene script.");
		return 1;
	}

	return 0;
}

void Scene::apply_modifiers()
{
	std::string mod;
/*
	while (Environment::handle().modifier_pop(mod)) {

		// Check if there is actually an assignment
		if (mod.find_last_of(':') == std::string::npos) {
			Log::handle().log_warning("Invalid rule: %s", mod.c_str());
			continue;
		}

		NCF *node = &m_scene;

		std::string nleft, nright;
		while((mod.find_first_of('.') != std::string::npos) && (mod.find_first_of(':') > mod.find_first_of('.'))) {
			Util::String::split(mod, nleft, nright, '.');
			mod = nright;

			// move to node
			node = node->group(nleft.c_str());
		}

		Util::String::split(mod, nleft, nright, ':');
		node->set(nleft.c_str(), nright.c_str());
	}
*/
}

unsigned int Scene::build()
{
	Log::handle().log_message("Setting up the scene environment..");

	// Start populating the lists
	unsigned int count = 0;

	scalar_t k = to_double(m_scene.get_property_by_name(XTPROTO_PROP_KAMBN));
	m_ambient = deserialize_col3(m_scene.get_group_by_name(XTPROTO_PROP_IAMBN))
			  * (k < 0.f ? 0.f : k);

	std::list<std::string> sections;
	sections.push_back(XTPROTO_NODE_LIGHT);
	sections.push_back(XTPROTO_NODE_MATERIAL);
	sections.push_back(XTPROTO_NODE_TEXTURE);
	sections.push_back(XTPROTO_NODE_GEOMETRY);
	sections.push_back(XTPROTO_NODE_OBJECT);

	std::list<std::string>::iterator it;

	Log::handle().log_message("Building the scene objects..");

	for (it = sections.begin(); it != sections.end(); it++)
	{
		// Populate the groups
		count = m_scene.get_group_by_name((*it).c_str())->count_groups();
		if (count)
		{
			Log::handle().log_message("Processing section: %s", (*it).c_str());
			for (unsigned int i = 0; i < count; ++i)
			{
				NCF *lnode = m_scene.get_group_by_name((*it).c_str())->get_group_by_index(i);

				// Handle node
				     if (!(*it).compare(XTPROTO_NODE_OBJECT   )) create_object(lnode);
				else if (!(*it).compare(XTPROTO_NODE_LIGHT    )) create_light(lnode);
				else if (!(*it).compare(XTPROTO_NODE_MATERIAL )) create_material(lnode);
				else if (!(*it).compare(XTPROTO_NODE_TEXTURE  )) create_texture(lnode);
				else if (!(*it).compare(XTPROTO_NODE_GEOMETRY )) create_geometry(lnode);
			}
		}
	}
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

unsigned int Scene::set_camera(const char *name)
{
	// Check if there are no cameras in the scene
	if(m_scene.get_group_by_name(XTPROTO_NODE_CAMERA)->count_groups() < 1)
	{
		Log::handle().log_message("No cameras were specified. Nothing to render..");
		return 1;
	}

	// Try the given -> default -> first available camera
	std::string dcam;

	// I do this in order to avoid exception:
	//		basic_string::_S_construct null not valid
	// in case of: name == NULL
	if (name) dcam = name;

	if (dcam.empty()) {
		dcam = m_scene.get_group_by_name(XTPROTO_NODE_CAMERA)->get_property_by_name(XTPROTO_PROP_DEFAULT);
		if (dcam.empty()) {
			Log::handle().log_message("No default camera was specified.");
			dcam = m_scene.get_group_by_name(XTPROTO_NODE_CAMERA)->get_group_by_index(0)->get_name();
		}
	}

	// Check if camera exists
	if(!m_scene.get_group_by_name(XTPROTO_NODE_CAMERA)->query_group(dcam.c_str())) {
		Log::handle().log_warning("Invalid camera: %s", dcam.c_str());
		dcam = m_scene.get_group_by_name(XTPROTO_NODE_CAMERA)->get_group_by_index(0)->get_name();
	}

	Log::handle().log_message("Using camera: %s", dcam.c_str());

	if (camera) delete camera;

	NCF *node = m_scene.get_group_by_name(XTPROTO_NODE_CAMERA)->get_group_by_name(dcam.c_str());

	NMath::Vector3f pos     = deserialize_vec3(node->get_group_by_name(XTPROTO_PROP_POSITION));
	NMath::Vector3f target  = deserialize_vec3(node->get_group_by_name(XTPROTO_PROP_TARGET));
	NMath::Vector3f up      = deserialize_vec3(node->get_group_by_name(XTPROTO_PROP_UP));
	NMath::scalar_t flength = deserialize_numf(node->get_property_by_name(XTPROTO_PROP_FLENGTH));
	NMath::scalar_t app     = deserialize_numf(node->get_property_by_name(XTPROTO_PROP_APERTURE));
	NMath::scalar_t fov     = deserialize_numf(node->get_property_by_name(XTPROTO_PROP_FOV));
	camera = new Camera(pos, target, up, fov, app, flength);

	return 0;
}

unsigned int Scene::create_light(NCF *p)
{
	if (!p)
		return 1;

	std::string type = p->get_property_by_name(XTPROTO_PROP_TYPE);

	Light *light = NULL;

	if (!type.compare(XTPROTO_LTRL_POINTLIGHT)) {
		light = new (std::nothrow) PointLight;

		if (!light)
			return 2;
	}
	else if (!type.compare(XTPROTO_LTRL_SPHERELIGHT)) {
		light = new (std::nothrow) SphereLight;
		if (!light)
			return 2;

		scalar_t radius = (scalar_t)to_double(p->get_property_by_name(XTPROTO_PROP_RADIUS));
		((SphereLight *)light)->radius(radius);
	}
	else if (!type.compare(XTPROTO_LTRL_BOXLIGHT)) {
		light = new (std::nothrow) BoxLight;

		NMath::Vector3f dimensions = deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_DIMENSIONS));
		((BoxLight *)light)->dimensions(dimensions);

		if (!light)
			return 2;
	}
	else if (!type.compare(XTPROTO_LTRL_TRIANGLELIGHT)) {
		light = new (std::nothrow) TriangleLight;

		Vector3f v[3];
		for (unsigned int i = 0; i < 3; ++i) {
			NCF *vnode = p->get_group_by_name(XTPROTO_PROP_VRTXDATA)->get_group_by_index(i);
			v[i] = deserialize_vec3(vnode);
		}

		((TriangleLight*)light)->geometry(v[0], v[1], v[2]);
	}
	else {
		Log::handle().log_warning("Unsupported light type %s [%s]. Skipping..", p->get_name(), type.c_str());
		return 2;
	}

	// Set the common properties.
	light->intensity(deserialize_col3(p->get_group_by_name(XTPROTO_PROP_INTST)));

	// Extract the properties.
	NMath::Vector3f position = deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_POSITION));
	light->position(position);

	// Destroy the old instance (if one exists).
	unsigned int res = destroy_light(p->get_name());

	// Add it to the list.
	m_lights[p->get_name()] = light;

	return res ? 0 : 3;
}

unsigned int Scene::create_geometry(NCF *p)
{
	if (!p)
		return 1;

	Geometry *geometry = NULL;

	std::string type = p->get_property_by_name(XTPROTO_PROP_TYPE);
	// - Plane.
	if (!type.compare(XTPROTO_LTRL_PLANE)) {
		geometry = new (std::nothrow) Plane;

		if(!geometry)
			return 2;

		// Set the properties.
		((Plane *)geometry)->normal   = deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_NORMAL));
		((Plane *)geometry)->distance = (scalar_t)to_double(p->get_property_by_name(XTPROTO_PROP_DISTANCE));
	}
	// - Sphere.
	else if (!type.compare(XTPROTO_LTRL_SPHERE)) {
		geometry = new (std::nothrow) Sphere;

		if(!geometry)
			return 2;

		// set the properties.
		((Sphere *)geometry)->origin = deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_POSITION));
		scalar_t radius	= (scalar_t)to_double(p->get_property_by_name(XTPROTO_PROP_RADIUS));
		((Sphere *)geometry)->radius = radius;
	}
	// - Triangle.
	else if (!type.compare(XTPROTO_LTRL_TRIANGLE)) {
		geometry = new (std::nothrow) Triangle;

		if(!geometry)
			return 2;

		for (unsigned int i = 0; i < 3; ++i) {
			NCF *vnode = p->get_group_by_name(XTPROTO_PROP_VRTXDATA)->get_group_by_index(i);
			((Triangle *)geometry)->v[i]  = deserialize_vec3(vnode);
			((Triangle *)geometry)->tc[i] = deserialize_tex2(vnode);
		}
	}
	// - Mesh
	else if (!type.compare(XTPROTO_LTRL_MESH)) {
		geometry = new (std::nothrow) Mesh;

		// Smooth surface
		bool smooth = to_bool(p->get_property_by_name(XTPROTO_PROP_SMOOTH));

		// Data source
		std::string f = p->get_property_by_name(XTPROTO_PROP_SOURCE);

		// Open source file from relative path
		std::string base, file;
		path_comp(m_scene.get_source(), base, file);
		base.append(f);

		Log::handle().log_message("Loading data from %s", base.c_str());

		if (NMesh::IO::Import::obj(base.c_str(), dynamic_cast<NMesh::Mesh &>(*geometry)))
		{
			Log::handle().log_warning("Failed to load mesh from %s", f.c_str());
			delete geometry;
			return 1;
		}

		if (p->query_group(XTPROTO_PROP_SCALE)) {
			NMath::Vector3f v = deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_SCALE));
			NMesh::Mutator::scale(dynamic_cast<NMesh::Mesh &>(*geometry), v.x, v.y, v.z);
		}

		if (p->query_group(XTPROTO_PROP_TRANSLATION)) {
			NMath::Vector3f v = deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_TRANSLATION));
			NMesh::Mutator::translate(dynamic_cast<NMesh::Mesh &>(*geometry), v.x, v.y, v.z);
		}

		if (smooth) {
			Log::handle().log_message("Recalculating normals..");
			NMesh::Mutator::calc_normals(dynamic_cast<NMesh::Mesh &>(*geometry));
		}

		Log::handle().log_message("Building octree..");
		((Mesh *)geometry)->build_octree();
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

	std::string type = p->get_property_by_name(XTPROTO_PROP_TYPE);

	     if (!type.compare(XTPROTO_LTRL_LAMBERT)   ) material->type = MATERIAL_LAMBERT;
	else if (!type.compare(XTPROTO_LTRL_PHONG)     ) material->type = MATERIAL_PHONG;
	else if (!type.compare(XTPROTO_LTRL_BLINNPHONG)) material->type = MATERIAL_BLINNPHONG;
	else {
		Log::handle().log_warning("Unsupported material %s. Skipping..", p->get_name());
		delete material;
		return 1;
	}

	material->ambient      = deserialize_col3(p->get_group_by_name(XTPROTO_PROP_IAMBN));
	material->specular     = deserialize_col3(p->get_group_by_name(XTPROTO_PROP_ISPEC));
	material->diffuse      = deserialize_col3(p->get_group_by_name(XTPROTO_PROP_IDIFF));
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
	if (!p)
		return 1;

	Texture2D *texture = new (std::nothrow) Texture2D;

	if (!texture)
		return 2;

	std::string source = p->get_property_by_name(XTPROTO_PROP_SOURCE);

	std::string script_source = m_scene.get_source();
	std::string script_base, script_filename;
	path_comp(script_source, script_base, script_filename);

	source = script_base + source;

	Log::handle().log_message("Loading data from %s", source.c_str());
	if (texture->load(source.c_str())) {
		Log::handle().log_warning("Failed to load texture [%s->%s]", p->get_name(), source.c_str());

		delete texture;
		return 1;
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
			object->geometry = n;
		}
		else {
			Log::handle().log_warning("At object: %s, geometry %s does not exist.", p->get_name(), n);
			delete object;
			return 5;
		}
	}

	{
		const char *n = p->get_property_by_name(XTPROTO_PROP_OBJ_MAT);
		if (m_materials.find(n) != m_materials.end()) {
			object->material = n;
		}
		else {
			Log::handle().log_warning("At object: %s, material %s does not exist.", p->get_name(), n);
			delete object;
			return 5;
		}
	}

	{
		const char *n = p->get_property_by_name(XTPROTO_PROP_OBJ_TEX);

		if (n) {
			if(m_textures.find(n) != m_textures.end()) {
				object->texture = n;
			}
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

bool Scene::intersection(const Ray &ray, IntInfo &info, std::string &obj)
{
	IntInfo test, res;

	std::map<std::string, Object *>::iterator it;
	for (it = m_objects.begin(); it != m_objects.end(); it++) {
		// test all the objects and find the closest intersection
		if((m_geometry[(*it).second->geometry.c_str()])->intersection(ray, &test)) {
			if(res.t > test.t) {
				// set the object name
				obj = (*it).first;

				// copy intinfo
				memcpy(&res, &test, sizeof(res));
			}
		}
	}

	// copy result to info
	memcpy(&info, &res, sizeof(info));

	return info.t != INFINITY ? true : false;
}
