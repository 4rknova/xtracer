/*

    This file is part of xtracer.

    scene.cpp
    Scene class

    Copyright (C) 2010, 2011
    Papadopoulos Nikolaos

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General
    Public License along with this program; if not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301 USA

*/

#include <vector>
#include <string>
#include <fstream>

#include <ncf/util.hpp>

#include <nmesh/transform.hpp>
#include <nmesh/calcnormals.hpp>
#include <nmesh/obj.hpp>

#include <nmath/mutil.h>
#include <nmath/vector.h>
#include <nmath/geometry.h>
#include <nmath/sphere.h>
#include <nmath/plane.h>
#include <nmath/triangle.h>
#include "argparse.hpp"
#include "mesh.hpp"
#include "proto.h"
#include "log.hpp"
#include "scene.hpp"

using NMath::Vector2f;
using NMath::Vector3f;
using NMath::Geometry;
using NMath::Plane;
using NMath::Triangle;
using NMath::Sphere;
using NCF::Util::path_comp;
using NCF::Util::trim;
using NCF::Util::split;
using NCF::Util::to_double;
using NCF::Util::to_bool;

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

const char *Scene::name()
{
	return m_scene.get(XTPROTO_PROP_TITLE);
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
	m_scene.source(filename);

	if (m_scene.parse())  {
		Log::handle().log_error("Failed to parse the scene script.");
		return 1;
	}

	return 0;
}

void Scene::apply_modifiers()
{
	std::string mod;
	while (Environment::handle().modifier_pop(mod)) {

		// Check if there is actually an assignment
		if (mod.find_last_of(':') == std::string::npos) {
			Log::handle().log_warning("Invalid rule: %s", mod.c_str());
			continue;
		}

		NCF1 *node = &m_scene;

		std::string nleft, nright;
		while((mod.find_first_of('.') != std::string::npos) && (mod.find_first_of(':') > mod.find_first_of('.'))) {
			NCF::Util::split(mod, nleft, nright, '.');
			mod = nright;

			// move to node
			node = node->group(nleft.c_str());
		}

		NCF::Util::split(mod, nleft, nright, ':');
		node->set(nleft.c_str(), nright.c_str());
	}
}

unsigned int Scene::build()
{
	Log::handle().log_message("Setting up the scene environment..");

	// Start populating the lists
	unsigned int count = 0;

	scalar_t r, g, b, k;
	r = to_double(m_scene.group(XTPROTO_PROP_IAMBN)->get(XTPROTO_PROP_COL_R));
	g = to_double(m_scene.group(XTPROTO_PROP_IAMBN)->get(XTPROTO_PROP_COL_G));
	b = to_double(m_scene.group(XTPROTO_PROP_IAMBN)->get(XTPROTO_PROP_COL_B));
 	k = to_double(m_scene.get(XTPROTO_PROP_KAMBN));

	m_ambient = ColorRGBf(r, g, b) * (k < 0.f ? 0.f : k);

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
		count = m_scene.group((*it).c_str())->count_groups();
		if (count)
		{
			Log::handle().log_message("Processing section: %s", (*it).c_str());
			for (unsigned int i = 1; i<= count; i++)
			{
				NCF1 *lnode = m_scene.group((*it).c_str())->group(i);

				// Handle node
				if (!(*it).compare(XTPROTO_NODE_OBJECT))
					create_object(lnode);
				else if (!(*it).compare(XTPROTO_NODE_LIGHT))
					create_light(lnode);
				else if (!(*it).compare(XTPROTO_NODE_MATERIAL))
					create_material(lnode);
				else if (!(*it).compare(XTPROTO_NODE_TEXTURE))
					create_texture(lnode);
				else if (!(*it).compare(XTPROTO_NODE_GEOMETRY))
					create_geometry(lnode);
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
	if(m_scene.group(XTPROTO_NODE_CAMERA)->count_groups() < 1)
	{
		Log::handle().log_message("No cameras were specified. Nothing to render..");
		return 1;
	}

	// Try the given -> default -> first available camera
	std::string dcam; 
	
	// I do this in order to avoid exception: 
	//		basic_string::_S_construct null not valid
	// in case of: name == NULL
	if (name)
		dcam = name;

	if (dcam.empty())
	{
		dcam = m_scene.group(XTPROTO_NODE_CAMERA)->get(XTPROTO_PROP_DEFAULT);
		if (dcam.empty())
		{
			Log::handle().log_message("No default camera was specified.");
			dcam = m_scene.group(XTPROTO_NODE_CAMERA)->group(1)->name();
		}   
	}

	// Check if camera exists
	if(!m_scene.group(XTPROTO_NODE_CAMERA)->query_group(dcam.c_str()))
	{
		Log::handle().log_warning("Invalid camera: %s", dcam.c_str());
		dcam = m_scene.group(XTPROTO_NODE_CAMERA)->group(1)->name();
	}   

	Log::handle().log_message("Using camera: %s", dcam.c_str());

	// extract the camera properties
	// fov
	std::string f = m_scene.group(XTPROTO_NODE_CAMERA)->group(dcam.c_str())->get(XTPROTO_PROP_FOV);
	scalar_t fov = to_double(f);

	// position
	std::string x = m_scene.group(XTPROTO_NODE_CAMERA)->group(dcam.c_str())->group(XTPROTO_PROP_POSITION)->get(XTPROTO_PROP_CRD_X);
	std::string y = m_scene.group(XTPROTO_NODE_CAMERA)->group(dcam.c_str())->group(XTPROTO_PROP_POSITION)->get(XTPROTO_PROP_CRD_Y);
	std::string z = m_scene.group(XTPROTO_NODE_CAMERA)->group(dcam.c_str())->group(XTPROTO_PROP_POSITION)->get(XTPROTO_PROP_CRD_Z);
	Vector3f pos(to_double(x), to_double(y), to_double(z));

	// target
	x = m_scene.group(XTPROTO_NODE_CAMERA)->group(dcam.c_str())->group(XTPROTO_PROP_TARGET)->get(XTPROTO_PROP_CRD_X);
	y = m_scene.group(XTPROTO_NODE_CAMERA)->group(dcam.c_str())->group(XTPROTO_PROP_TARGET)->get(XTPROTO_PROP_CRD_Y);
	z = m_scene.group(XTPROTO_NODE_CAMERA)->group(dcam.c_str())->group(XTPROTO_PROP_TARGET)->get(XTPROTO_PROP_CRD_Z);
	Vector3f targ(to_double(x), to_double(y), to_double(z));

	// up vector
	x = m_scene.group(XTPROTO_NODE_CAMERA)->group(dcam.c_str())->group(XTPROTO_PROP_UP)->get(XTPROTO_PROP_CRD_X);
	y = m_scene.group(XTPROTO_NODE_CAMERA)->group(dcam.c_str())->group(XTPROTO_PROP_UP)->get(XTPROTO_PROP_CRD_Y);
	z = m_scene.group(XTPROTO_NODE_CAMERA)->group(dcam.c_str())->group(XTPROTO_PROP_UP)->get(XTPROTO_PROP_CRD_Z);
	Vector3f up(to_double(x), to_double(y), to_double(z));

	// aperture
	scalar_t app = to_double(m_scene.group(XTPROTO_NODE_CAMERA)->group(dcam.c_str())->get(XTPROTO_PROP_APERTURE));

	// focal length
	scalar_t flength = to_double(m_scene.group(XTPROTO_NODE_CAMERA)->group(dcam.c_str())->get(XTPROTO_PROP_FLENGTH));

	// release the previous camera if needed
	if (camera)
		delete camera;

	// create the camera
	camera = new Camera(pos, targ, up, fov, app, flength);

	return 0;
}

unsigned int Scene::create_light(NCF1 *p)
{
	if (!p)
		return 1;

	// Extract the properties.
	scalar_t posx = to_double(p->group(XTPROTO_PROP_POSITION)->get(XTPROTO_PROP_CRD_X));
	scalar_t posy = to_double(p->group(XTPROTO_PROP_POSITION)->get(XTPROTO_PROP_CRD_Y));
	scalar_t posz = to_double(p->group(XTPROTO_PROP_POSITION)->get(XTPROTO_PROP_CRD_Z));
	
	scalar_t colr = to_double(p->group(XTPROTO_PROP_INTST)->get(XTPROTO_PROP_COL_R));
	scalar_t colg = to_double(p->group(XTPROTO_PROP_INTST)->get(XTPROTO_PROP_COL_G));
	scalar_t colb = to_double(p->group(XTPROTO_PROP_INTST)->get(XTPROTO_PROP_COL_B));

	std::string type = p->get(XTPROTO_PROP_TYPE);

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
		
		scalar_t radius = (scalar_t)to_double(p->get(XTPROTO_PROP_RADIUS));
		((SphereLight *)light)->radius(radius);
	}
	else if (!type.compare(XTPROTO_LTRL_BOXLIGHT)) {
		light = new (std::nothrow) BoxLight;

		scalar_t dimx = (scalar_t)to_double(p->group(XTPROTO_PROP_DIMENSIONS)->get(XTPROTO_PROP_CRD_X)); 
		scalar_t dimy = (scalar_t)to_double(p->group(XTPROTO_PROP_DIMENSIONS)->get(XTPROTO_PROP_CRD_Y));
		scalar_t dimz = (scalar_t)to_double(p->group(XTPROTO_PROP_DIMENSIONS)->get(XTPROTO_PROP_CRD_Z));

		((BoxLight *)light)->dimensions(Vector3f(dimx, dimy, dimz));

		if (!light)
			return 2;
	}
	else if (!type.compare(XTPROTO_LTRL_TRIANGLELIGHT)) {
		light = new (std::nothrow) TriangleLight;

		Vector3f v[3];
		for (unsigned int i = 1; i <= 3; i++) {
			NCF1 *vnode = p->group(XTPROTO_PROP_VRTXDATA)->group(i);
			
			scalar_t px = (scalar_t)to_double(vnode->get(XTPROTO_PROP_CRD_X));
			scalar_t py = (scalar_t)to_double(vnode->get(XTPROTO_PROP_CRD_Y));
			scalar_t pz = (scalar_t)to_double(vnode->get(XTPROTO_PROP_CRD_Z));
			
			v[i-1] = Vector3f(px, py, pz);
		}

		((TriangleLight*)light)->geometry(v[0], v[1], v[2]);
	}
	else {
		Log::handle().log_warning("Unsupported light type %s [%s]. Skipping..", p->name(), type.c_str());
		return 2;
	}

	// Set the common properties.
	light->position(Vector3f(posx, posy, posz));
	light->intensity(ColorRGBf(colr, colg, colb));

	// Destroy the old instance (if one exists).
	unsigned int res = destroy_light(p->name());

	// Add it to the list.
	m_lights[p->name()] = light;

	return res ? 0 : 3;
}

unsigned int Scene::create_geometry(NCF1 *p)
{
	if (!p)
		return 1;

	Geometry *geometry = NULL;

	std::string type = p->get(XTPROTO_PROP_TYPE);

	// - Plane.
	if (!type.compare(XTPROTO_LTRL_PLANE)) {
		scalar_t normx		= (scalar_t)to_double(p->group(XTPROTO_PROP_NORMAL)->get(XTPROTO_PROP_CRD_X));
		scalar_t normy 		= (scalar_t)to_double(p->group(XTPROTO_PROP_NORMAL)->get(XTPROTO_PROP_CRD_Y));
		scalar_t normz		= (scalar_t)to_double(p->group(XTPROTO_PROP_NORMAL)->get(XTPROTO_PROP_CRD_Z));
		scalar_t distance 	= (scalar_t)to_double(p->get(XTPROTO_PROP_DISTANCE));

		geometry = new (std::nothrow) Plane;

		if(!geometry)
			return 2;

		// Set the properties.
		((Plane *)geometry)->normal = Vector3f(normx, normy, normz);
		((Plane *)geometry)->distance = distance;
	}
	// - Sphere.
	else if (!type.compare(XTPROTO_LTRL_SPHERE)) {
		scalar_t posx	= (scalar_t)to_double(p->group(XTPROTO_PROP_POSITION)->get(XTPROTO_PROP_CRD_X));
		scalar_t posy	= (scalar_t)to_double(p->group(XTPROTO_PROP_POSITION)->get(XTPROTO_PROP_CRD_Y));
		scalar_t posz	= (scalar_t)to_double(p->group(XTPROTO_PROP_POSITION)->get(XTPROTO_PROP_CRD_Z));
		scalar_t radius	= (scalar_t)to_double(p->get(XTPROTO_PROP_RADIUS));

		geometry = new (std::nothrow) Sphere;

		if(!geometry)
			return 2;
		
		// set the properties.
		((Sphere *)geometry)->origin = Vector3f(posx, posy, posz);
		((Sphere *)geometry)->radius = radius;
	}	
	// - Triangle.
	else if (!type.compare(XTPROTO_LTRL_TRIANGLE)) {
		geometry = new (std::nothrow) Triangle;

		if(!geometry)
			return 2;
		
		for (unsigned int i = 1; i <= 3; i++) {
			NCF1 *vnode = p->group(XTPROTO_PROP_VRTXDATA)->group(i);

			scalar_t px = (scalar_t)to_double(vnode->get(XTPROTO_PROP_CRD_X));
			scalar_t py = (scalar_t)to_double(vnode->get(XTPROTO_PROP_CRD_Y));
			scalar_t pz = (scalar_t)to_double(vnode->get(XTPROTO_PROP_CRD_Z));
			scalar_t tu = (scalar_t)to_double(vnode->get(XTPROTO_PROP_CRD_U));
			scalar_t tv = (scalar_t)to_double(vnode->get(XTPROTO_PROP_CRD_V));

			((Triangle *)geometry)->v[i-1] = Vector3f(px, py, pz);
			((Triangle *)geometry)->tc[i-1] = Vector2f(tu, tv);
		}
	}
	// - Mesh
	else if (!type.compare(XTPROTO_LTRL_MESH)) {
		geometry = new (std::nothrow) Mesh;
		
		// Smooth surface
		bool smooth = to_bool(p->get(XTPROTO_PROP_SMOOTH));
		
		// Data source
		std::string f = p->get(XTPROTO_PROP_SOURCE);

		// Open source file from relative path
		std::string base, file;
		path_comp(m_scene.source(), base, file);
		base.append(f);

		Log::handle().log_message("Loading data from %s", base.c_str());

		if (NMesh::IO::Import::obj(base.c_str(), dynamic_cast<NMesh::Mesh &>(*geometry)))
		{
			Log::handle().log_warning("Failed to load mesh from %s", f.c_str());
			delete geometry;
			return 1;
		}
		
		if (p->query_group(XTPROTO_PROP_SCALE)) {
			NMesh::Mutator::scale(dynamic_cast<NMesh::Mesh &>(*geometry),
				(scalar_t)to_double(p->group(XTPROTO_PROP_SCALE)->get(XTPROTO_PROP_CRD_X)),
				(scalar_t)to_double(p->group(XTPROTO_PROP_SCALE)->get(XTPROTO_PROP_CRD_Y)),
				(scalar_t)to_double(p->group(XTPROTO_PROP_SCALE)->get(XTPROTO_PROP_CRD_Z)));
		}

		if (p->query_group(XTPROTO_PROP_TRANSLATION)) {
			NMesh::Mutator::translate(dynamic_cast<NMesh::Mesh &>(*geometry),
				(scalar_t)to_double(p->group(XTPROTO_PROP_TRANSLATION)->get(XTPROTO_PROP_CRD_X)),
				(scalar_t)to_double(p->group(XTPROTO_PROP_TRANSLATION)->get(XTPROTO_PROP_CRD_Y)),
				(scalar_t)to_double(p->group(XTPROTO_PROP_TRANSLATION)->get(XTPROTO_PROP_CRD_Z)));
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
		Log::handle().log_warning("Unsupported geometry type %s [%s]. Skipping..", p->name(), type.c_str());
		return 1;
	}
	
	scalar_t u_scale = (scalar_t)to_double(p->get(XTPROTO_PROP_USCALE));
	scalar_t v_scale = (scalar_t)to_double(p->get(XTPROTO_PROP_VSCALE));
	geometry->uv_scale = Vector2f(u_scale, v_scale);

	geometry->calc_aabb();

	// Destroy the old instance (if one exists).
	unsigned int res = destroy_geometry(p->name());

	// Add it to the list.
	m_geometry[p->name()] = geometry;
	
	return res ? 0 : 3;
}

unsigned int Scene::create_material(NCF1 *p)
{
	if (!p)
		return 1;

	Material *material = new (std::nothrow) Material;

	if (!material)
		return 2;

	std::string type = p->get(XTPROTO_PROP_TYPE);

	if (!type.compare(XTPROTO_LTRL_LAMBERT)) {
		material->type = MATERIAL_LAMBERT;
	}
	else if (!type.compare(XTPROTO_LTRL_PHONG)) {
		material->type = MATERIAL_PHONG;
	}
	else if (!type.compare(XTPROTO_LTRL_BLINNPHONG)) {
		material->type = MATERIAL_BLINNPHONG;
	}
	else {
		Log::handle().log_warning("Unsupported material %s. Skipping..", p->name());
		delete material;
		return 1;
	}

	// ambient intensity
	scalar_t ambr = (scalar_t)to_double(p->group(XTPROTO_PROP_IAMBN)->get(XTPROTO_PROP_COL_R));
	scalar_t ambg = (scalar_t)to_double(p->group(XTPROTO_PROP_IAMBN)->get(XTPROTO_PROP_COL_G));
	scalar_t ambb = (scalar_t)to_double(p->group(XTPROTO_PROP_IAMBN)->get(XTPROTO_PROP_COL_B));

	material->ambient = ColorRGBf(ambr, ambg, ambb);

	// specular intensity
	scalar_t specr = (scalar_t)to_double(p->group(XTPROTO_PROP_ISPEC)->get(XTPROTO_PROP_COL_R));
	scalar_t specg = (scalar_t)to_double(p->group(XTPROTO_PROP_ISPEC)->get(XTPROTO_PROP_COL_G));
	scalar_t specb = (scalar_t)to_double(p->group(XTPROTO_PROP_ISPEC)->get(XTPROTO_PROP_COL_B));

	material->specular = ColorRGBf(specr, specg, specb);


	 if (!type.compare(XTPROTO_LTRL_LAMBERT)) {
		material->kdiff = 1;
		material->kspec = 0;
	 }
	 else {
		material->kdiff = (scalar_t)to_double(p->get(XTPROTO_PROP_KDIFF));
		material->kspec = (scalar_t)to_double(p->get(XTPROTO_PROP_KSPEC));
		material->ksexp = (scalar_t)to_double(p->get(XTPROTO_PROP_KEXPN));
	 }

	material->roughness = (scalar_t)to_double(p->get(XTPROTO_PROP_ROUGH));

	// diffuse intensity
	scalar_t diffr = (scalar_t)to_double(p->group(XTPROTO_PROP_IDIFF)->get(XTPROTO_PROP_COL_R));
	scalar_t diffg = (scalar_t)to_double(p->group(XTPROTO_PROP_IDIFF)->get(XTPROTO_PROP_COL_G));
	scalar_t diffb = (scalar_t)to_double(p->group(XTPROTO_PROP_IDIFF)->get(XTPROTO_PROP_COL_B));
		
	material->diffuse = ColorRGBf(diffr, diffg, diffb);
	
	material->reflectance  = NMath::saturate((scalar_t)to_double(p->get(XTPROTO_PROP_REFLC)));
	material->transparency = NMath::saturate((scalar_t)to_double(p->get(XTPROTO_PROP_TRSPC)));

	// index of refraction
	material->ior = (scalar_t)to_double(p->get(XTPROTO_PROP_IOR));

	// Destroy the old material from the list if it exists.
	unsigned int res = destroy_material(p->name());

	// Add the material to the list.
	m_materials[p->name()] = material;
 
	return res ? 0 : 3;
}

unsigned int Scene::create_texture(NCF1 *p)
{
	if (!p)
		return 1;

	Texture2D *texture = new (std::nothrow) Texture2D;

	if (!texture)
		return 2;

	std::string source = p->get(XTPROTO_PROP_SOURCE);

	std::string script_source = m_scene.source();
	std::string script_base, script_filename;
	path_comp(script_source, script_base, script_filename);

	source = script_base + source;	

	Log::handle().log_message("Loading data from %s", source.c_str());
	if (texture->load(source.c_str())) {
		Log::handle().log_warning("Failed to load texture [%s->%s]", p->name(), source.c_str());

		delete texture;
		return 1;
	}

	// Destroy the old texture from the list if it exists.
	unsigned int res = destroy_texture(p->name());

	// Add the texture to the list.
	m_textures[p->name()] = texture;

	return res ? 0 : 3;

}

unsigned int Scene::create_object(NCF1 *p)
{
	if (!p)
		return 1;

	Object *object = new (std::nothrow) Object;

	std::string n;
	
	n = p->get(XTPROTO_PROP_OBJ_GEO);
	if(m_geometry.find(n) != m_geometry.end()) {
		object->geometry = n;
	}
	else {
		Log::handle().log_warning("At object: %s, geometry %s does not exist.", p->name(), n.c_str());
		delete object;
		return 5;
	}

	n = p->get(XTPROTO_PROP_OBJ_MAT);
	if(m_materials.find(n) != m_materials.end()) {
		object->material = n;
	}
	else {
		Log::handle().log_warning("At object: %s, material %s does not exist.", p->name(), n.c_str());
		delete object;
		return 5;
	}

	n = p->get(XTPROTO_PROP_OBJ_TEX);

	if (!n.empty()) {
		if(m_textures.find(n) != m_textures.end()) {
			object->texture = n;
		}
		else {
			Log::handle().log_warning("At object: %s, texture %s does not exist.", p->name(), n.c_str());
			delete object;
			return 5;
		}
	}

	// Destroy the old instance (if one exists).
	unsigned int res = destroy_object(p->name());

	// Add the object to the list
	m_objects[p->name()] = object;
	
	return res ? 0 : 3;
}

bool Scene::intersection(const Ray &ray, IntInfo &info, std::string &obj)
{
	IntInfo test, res;

	std::map<std::string, Object *>::iterator it;
	for (it = m_objects.begin(); it != m_objects.end(); it++)
	{
		// test all the objects and find the closest intersection
		if((m_geometry[(*it).second->geometry.c_str()])->intersection(ray, &test))
		{
			if(res.t > test.t)
			{
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
