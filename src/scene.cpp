/*

    This file is part of xtracer.

    scene.cpp
    Scene class

    Copyright (C) 2010, 2011
    Papadopoulos Nikolaos

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General
    Public License along with this library; if not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301 USA

*/

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>

#include <nmath/vector.h>
#include <nmath/geometry.h>
#include <nparse/util.hpp>

#include "scene.hpp"
#include "proto.h"

Scene::Scene(const char *filepath): 
		camera(new Camera()),
		k_ambient(0.1),
		source(filepath),
		data(filepath)
{}

Scene::~Scene()
{
	cleanup();
}

void Scene::cleanup()
{

	std::cout << "Cleaning up..\n";

	// Release the lights
	if(!light.empty())
	{
		std::cout << "Releasing the lights..\n";
		for (std::map<std::string, Light *>::iterator it = light.begin(); it != light.end(); it++)
			delete (*it).second;
	}

	// Release the materials
	if(!material.empty())
	{
		std::cout << "Releasing the materials..\n";
		for (std::map<std::string, Material *>::iterator it = material.begin(); it != material.end(); it++)
			delete (*it).second;
	} 

	// Release the geometry
	if(!geometry.empty())
	{
		std::cout << "Releasing the geometry..\n";
		for (std::map<std::string, Geometry *>::iterator it = geometry.begin(); it != geometry.end(); it++)
			delete (*it).second;
	}

	// Release the objects
	if(!object.empty())
	{
		std::cout << "Releasing the objects..\n";
		for (std::map<std::string, Object *>::iterator it = object.begin(); it != object.end(); it++)
			delete (*it).second;
	} 

	// Release the camera
	std::cout << "Releasing the camera..\n";
	delete camera;
	camera = NULL;
}

unsigned int Scene::init()
{
	std::cout << "Initiating the scene..\n";

	// Load the scene file and parse it
	std::cout << "Parsing file.. \n";
	if(data.parse())
	{
		std::cerr << "Error: Failed to parse the scene file.\n";
		return 1;
	}
	return 0;
}

unsigned int Scene::build()
{
	std::cout << "Setting up the scene environment..\n";
	// Start populating the lists
	unsigned int count = 0;

	set_ambient();			// Set the scene ambient color

	std::list<std::string> sections;
	sections.push_back(XT_CFGPROTO_NODE_LIGHT);
	sections.push_back(XT_CFGPROTO_NODE_MATERIAL);
	sections.push_back(XT_CFGPROTO_NODE_GEOMETRY);
	sections.push_back(XT_CFGPROTO_NODE_OBJECT);

	std::list<std::string>::iterator it;

	std::cout << "Building scene data..\n";

	for (it = sections.begin(); it != sections.end(); it++)
	{
		// Populate the groups
		count = data.group((*it).c_str())->count_groups();
		if (count)
		{
			std::cout << "-> section: " << (*it).c_str() << "\n";
			for (unsigned int i = 1; i<= count; i++)
			{
				NCF1 *lnode = data.group((*it).c_str())->group(i);

				// Handle node
				if (!(*it).compare(XT_CFGPROTO_NODE_OBJECT))
					add_object(lnode);
				else if (!(*it).compare(XT_CFGPROTO_NODE_LIGHT))
					add_light(lnode);
				else if (!(*it).compare(XT_CFGPROTO_NODE_MATERIAL))
					add_material(lnode);
				else if (!(*it).compare(XT_CFGPROTO_NODE_GEOMETRY))
					add_geometry(lnode);
			}
		}
	}
	
	return 0;
}

unsigned int Scene::analyze()
{
	std::cout << "Analyzing scene data..\n";
	/* Print statistics */
	std::string pad = "    ";
	std::string name = data.get(XT_CFGPROTO_PROP_NAME);
	std::string info = data.get(XT_CFGPROTO_PROP_DESCRIPTION);

	if(!name.empty())
		std::cout 
			<< "Name: " << data.get(XT_CFGPROTO_PROP_NAME) << '\n';

	if(!info.empty())
		std::cout
			<< "Info: " << data.get(XT_CFGPROTO_PROP_DESCRIPTION) << "\n";

	std::string l;
	unsigned int c = 0;

	std::list<std::string> nodes;

	nodes.push_back(XT_CFGPROTO_NODE_CAMERA);
	nodes.push_back(XT_CFGPROTO_NODE_MATERIAL);
	nodes.push_back(XT_CFGPROTO_NODE_LIGHT);
	nodes.push_back(XT_CFGPROTO_NODE_GEOMETRY);
	nodes.push_back(XT_CFGPROTO_NODE_OBJECT);

	unsigned int n = nodes.size();

	unsigned int padl = pad.length();
	while(padl--) std::cout << '-';
	std::cout << '.' << '\n';

	std::cout << pad << "|\n";

	for (std::list<std::string>::iterator it = nodes.begin(); it != nodes.end(); it++)
	{
		n--;
		std::string node = (*it);
		c = data.group(node.c_str())->count_groups();
		if (c)
		{
			std::cout 
				<< pad << (n ? "|" : "'")
				<< "-> [" << c << "] " 
				<< node 
				<< "\n";

			for (unsigned int i = 1; i <= c; i++)
			{
				std::cout
					<< pad
					<< (n ? "|" : " ") << pad
					<< (i == c ? "'" : "|") << "-> " 
					<< data.group(node.c_str())->group(i)->node();

				if (!node.compare(XT_CFGPROTO_NODE_MATERIAL) || !node.compare(XT_CFGPROTO_NODE_GEOMETRY))
					std::cout
						<< "\n"	<< pad << (n ? "|" : " ")
						<< pad << (i == c ? " " : "|")
						<< pad << "type: "
						<<  data.group(node.c_str())->group(i)->get(XT_CFGPROTO_PROP_TYPE);
				else if (!node.compare(XT_CFGPROTO_NODE_OBJECT))
					std::cout
						<< "\n"	<< pad << (n ? "|" : " ")
						<< pad << (i == c ? " " : "|")
						<< pad << "geometry: "
						<< data.group(node.c_str())->group(i)->get(XT_CFGPROTO_PROP_GEOMETRY)
						<< "\n"	<< pad << (n ? "|" : " ")
						<< pad << (i == c ? " " : "|")
						<< pad << "material: "
						<< data.group(node.c_str())->group(i)->get(XT_CFGPROTO_PROP_MATERIAL);
				// TODO: Here list each item's properties
				
				std::cout << "\n";
			}

			std::cout << (n?pad:"") << (n ? "|" : "") << (n?"\n":"");
		}
	}

	return 0;
}

unsigned int Scene::set_ambient()
{
	std::string r, g, b, k;
	r = data.group(XT_CFGPROTO_PROP_AMBIENT)->get(XT_CFGPROTO_PROP_COLOR_R);
	g = data.group(XT_CFGPROTO_PROP_AMBIENT)->get(XT_CFGPROTO_PROP_COLOR_G);
	b = data.group(XT_CFGPROTO_PROP_AMBIENT)->get(XT_CFGPROTO_PROP_COLOR_B);

	k = data.get(XT_CFGPROTO_PROP_KAMBN);

	ambient = Vector3(nstring_to_double(r), nstring_to_double(g), nstring_to_double(b));
	k_ambient = nstring_to_double(k);
	return 0;
}

unsigned int Scene::set_camera(const char *name)
{
	// Check if there are no cameras in the scene
	if(data.group(XT_CFGPROTO_NODE_CAMERA)->count_groups() < 1)
	{
		std::cout << "No cameras were specified. Nothing to render..\n";
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
		dcam = data.group(XT_CFGPROTO_NODE_CAMERA)->get(XT_CFGPROTO_PROP_DEFAULT);
		if (dcam.empty())
		{
			std::cout << "Warning: No default camera was specified.\n";
			dcam = data.group(XT_CFGPROTO_NODE_CAMERA)->group(1)->node();
		}   
	}

	// Check if camera exists
	if(!data.group(XT_CFGPROTO_NODE_CAMERA)->query_group(dcam.c_str()))
	{
		std::cout << "Invalid camera: " << dcam << "\n";
		dcam = data.group(XT_CFGPROTO_NODE_CAMERA)->group(1)->node();
	}   

	std::cout << "Using camera: " << dcam << "\n";

	// extract the camera properties
	// fov
	std::string f = data.group(XT_CFGPROTO_NODE_CAMERA)->group(dcam.c_str())->get(XT_CFGPROTO_PROP_FOV);
	real_t fov = nstring_to_double(f);

	// position
	std::string x = data.group(XT_CFGPROTO_NODE_CAMERA)->group(dcam.c_str())->group(XT_CFGPROTO_PROP_POSITION)->get(XT_CFGPROTO_PROP_COORD_X);
	std::string y = data.group(XT_CFGPROTO_NODE_CAMERA)->group(dcam.c_str())->group(XT_CFGPROTO_PROP_POSITION)->get(XT_CFGPROTO_PROP_COORD_Y);
	std::string z = data.group(XT_CFGPROTO_NODE_CAMERA)->group(dcam.c_str())->group(XT_CFGPROTO_PROP_POSITION)->get(XT_CFGPROTO_PROP_COORD_Z);
	Vector3 pos(nstring_to_double(x), nstring_to_double(y), nstring_to_double(z));

	// target
	x = data.group(XT_CFGPROTO_NODE_CAMERA)->group(dcam.c_str())->group(XT_CFGPROTO_PROP_TARGET)->get(XT_CFGPROTO_PROP_COORD_X);
	y = data.group(XT_CFGPROTO_NODE_CAMERA)->group(dcam.c_str())->group(XT_CFGPROTO_PROP_TARGET)->get(XT_CFGPROTO_PROP_COORD_Y);
	z = data.group(XT_CFGPROTO_NODE_CAMERA)->group(dcam.c_str())->group(XT_CFGPROTO_PROP_TARGET)->get(XT_CFGPROTO_PROP_COORD_Z);
	Vector3 targ(nstring_to_double(x), nstring_to_double(y), nstring_to_double(z));

	// up vector
	x = data.group(XT_CFGPROTO_NODE_CAMERA)->group(dcam.c_str())->group(XT_CFGPROTO_PROP_UP)->get(XT_CFGPROTO_PROP_COORD_X);
	y = data.group(XT_CFGPROTO_NODE_CAMERA)->group(dcam.c_str())->group(XT_CFGPROTO_PROP_UP)->get(XT_CFGPROTO_PROP_COORD_Y);
	z = data.group(XT_CFGPROTO_NODE_CAMERA)->group(dcam.c_str())->group(XT_CFGPROTO_PROP_UP)->get(XT_CFGPROTO_PROP_COORD_Z);
	Vector3 up(nstring_to_double(x), nstring_to_double(y), nstring_to_double(z));

	// aperture
	real_t app = nstring_to_double(data.group(XT_CFGPROTO_NODE_CAMERA)->group(dcam.c_str())->get(XT_CFGPROTO_PROP_APERTURE));

	// shutter
	real_t shut = nstring_to_double(data.group(XT_CFGPROTO_NODE_CAMERA)->group(dcam.c_str())->get(XT_CFGPROTO_PROP_SHUTTER));

	// focal length
	real_t flength = nstring_to_double(data.group(XT_CFGPROTO_NODE_CAMERA)->group(dcam.c_str())->get(XT_CFGPROTO_PROP_FLENGTH));

	// cleanup the previous camera if needed
	if (camera)
		delete camera;

	// create the camera
	camera = new Camera(pos, targ, up, fov, app, flength, shut);

	return 0;
}

unsigned int Scene::add_light(NCF1 *p)
{
	// extract the light properties
	// position
	std::string posx = p->group(XT_CFGPROTO_PROP_POSITION)->get(XT_CFGPROTO_PROP_COORD_X);
	std::string posy = p->group(XT_CFGPROTO_PROP_POSITION)->get(XT_CFGPROTO_PROP_COORD_Y);
	std::string posz = p->group(XT_CFGPROTO_PROP_POSITION)->get(XT_CFGPROTO_PROP_COORD_Z);

	// intensity
	std::string r = p->group(XT_CFGPROTO_PROP_INTENSITY)->get(XT_CFGPROTO_PROP_COLOR_R);
	std::string g = p->group(XT_CFGPROTO_PROP_INTENSITY)->get(XT_CFGPROTO_PROP_COLOR_G);
	std::string b = p->group(XT_CFGPROTO_PROP_INTENSITY)->get(XT_CFGPROTO_PROP_COLOR_B);

	// Create the light
	Light *tlight = new Light();

	tlight->position = 
		Vector3(nstring_to_double(posx), 
				nstring_to_double(posy), 
				nstring_to_double(posz));
	tlight->intensity = 
		Vector3(nstring_to_double(r),
				nstring_to_double(g),
				nstring_to_double(b));

	light[p->node()] = tlight;

	return 0;
}

#include <nmath/sphere.h>
#include <nmath/plane.h>
#include <nmath/triangle.h>
#include <nmath/mesh.h>

unsigned int Scene::add_geometry(NCF1 *p)
{
	std::string type = p->get(XT_CFGPROTO_PROP_TYPE);

	Geometry *geo = NULL;

	// sphere
	if (!type.compare(XT_CFGPROTO_VAL_SPHERE))
	{
		geo = new Sphere();

		std::string x, y, z, r;

		x = p->group(XT_CFGPROTO_PROP_POSITION)->get(XT_CFGPROTO_PROP_COORD_X);
		y = p->group(XT_CFGPROTO_PROP_POSITION)->get(XT_CFGPROTO_PROP_COORD_Y);
		z = p->group(XT_CFGPROTO_PROP_POSITION)->get(XT_CFGPROTO_PROP_COORD_Z);
		r = p->get(XT_CFGPROTO_PROP_RADIUS);

		// set the position
		((Sphere*)geo)->origin = Vector3(
			nstring_to_double(x), 
			nstring_to_double(y), 
			nstring_to_double(z));

		// set the radius
		((Sphere *)geo)->radius = nstring_to_double(r);
	}
	// plane
	else if (!type.compare(XT_CFGPROTO_VAL_PLANE))
	{
		geo = new Plane();

		std::string x, y, z, d;

		x = p->group(XT_CFGPROTO_PROP_NORMAL)->get(XT_CFGPROTO_PROP_COORD_X);
		y = p->group(XT_CFGPROTO_PROP_NORMAL)->get(XT_CFGPROTO_PROP_COORD_Y);
		z = p->group(XT_CFGPROTO_PROP_NORMAL)->get(XT_CFGPROTO_PROP_COORD_Z);
		d = p->get(XT_CFGPROTO_PROP_DISTANCE);

		// set the normal
		((Plane *)geo)->normal = Vector3(
			nstring_to_double(x), 
			nstring_to_double(y), 
			nstring_to_double(z));

		// set the distance
		((Plane *)geo)->distance = nstring_to_double(d);
	}
	// triangle
	else if (!type.compare(XT_CFGPROTO_VAL_TRIANGLE))
	{
		geo = new Triangle();

		std::string x, y, z, d;

		unsigned int c = p->group(XT_CFGPROTO_PROP_VECDATA)->count_groups();

		if (c != 3)
		{
			std::cout 
				<< "Error: Geometry " << p->node()  
				<< " provided " << c << " vertices for a triangle.\n";

			delete geo;
			return 1;
		}

		for (unsigned int i = 1; i < c+1; i++)
		{
			x = p->group(XT_CFGPROTO_PROP_VECDATA)->group(i)->get(XT_CFGPROTO_PROP_COORD_X);
			y = p->group(XT_CFGPROTO_PROP_VECDATA)->group(i)->get(XT_CFGPROTO_PROP_COORD_Y);
			z = p->group(XT_CFGPROTO_PROP_VECDATA)->group(i)->get(XT_CFGPROTO_PROP_COORD_Z);

			((Triangle *)geo)->v[i-1].position = 
					Vector3(nstring_to_double(x),
							nstring_to_double(y),
							nstring_to_double(z));
		}
	}
	// mesh
	else if (!type.compare(XT_CFGPROTO_VAL_MESH))
	{
		geo = new Mesh();
		
		// Smooth surface
		bool smooth = false;
		std::string shading = p->get(XT_CFGPROTO_PROP_SMOOTH);

		if (!shading.compare(XT_CFGPROTO_VAL_TRUE))
			smooth = true;
		else if (!shading.compare(XT_CFGPROTO_VAL_FALSE)) 
			smooth = false;
		else
			std::cout 
				<< "Warning: [" 
				<<  p->node() 
				<< "->" 
				<< XT_CFGPROTO_PROP_SMOOTH << "] invalid value: " 
				<<  p->get(XT_CFGPROTO_PROP_SMOOTH) << "\n";
		
		// Data source
		std::string f = p->get(XT_CFGPROTO_PROP_SOURCE);

		// Open source file from relative path
		std::string base, file;
		nstring_path_comp(data.get_source(), base, file);
		base.append(f);

		std::cout << "Loading data from " << base << "..\n";
		std::fstream in(base.c_str(), std::ios::in);

		// Check for errors
		if (!in.good())
		{
			std::cout << "Warning: Failed to load mesh from " << f << ".\n";
			delete geo;
			return 1;
		}

		std::string line;
		unsigned int linec = 0;

		while (getline(in, line))
		{
			linec++;

			if(!line[0] || line[0] == '#') 
			{
				continue;
			}

			// Temporary buffers
			int vidx[3];
			float x, y, z;
			Vector3 vec;
			Vertex vertex;

			// Trim line off spaces
			nstring_trim(line);
			// Parse
			switch(line[0])
			{
				// vertices
				case 'v':
					if(sscanf(line.c_str(), "v %f %f %f", &x, &y, &z) < 3)
					{
						std::cout << "Syntax error in " << f << " at line " << linec << ".\n";
						delete geo;
						in.close();
						return 1;
					}
					vertex.position = Vector3(x, y, z);
					((Mesh *)geo)->vertices.push_back(vertex);
					break;
				// faces
				case 'f':
					int res = sscanf(line.c_str(), "f %d %d %d", vidx, vidx + 1, vidx + 2);
					if(res != 3)
					{
						std::cout << "Syntax error in " << base << " at line " << linec << ".\n";
						delete geo;
						in.close();
						return 1;
					}

					Face face;
					
					for(unsigned int i = 0; i < 3; i++)
					{
						face.v[i] = vidx[i] -1;
					}

					((Mesh *)geo)->faces.push_back(face);

					break;
			}
		}
		in.close();

		if (smooth)
		{
			// calculate vertex normal 
			std::cout << "Calculating vertex normals..\n";
			((Mesh*)geo)->calc_vertex_normals();
		}
	}
	// unknown
	else
	{
		std::cout << "Warning: Unsupported type " << p->node() << " [ " << type << " ]";

		if (!type.empty())
		{
			std::cout << ": " << type ;
		}

		std::cout << ". Ingoring..\n";

		return 1;
	}

	geo->calc_aabb();
	geometry[p->node()] = geo;

	return 0;
}

unsigned int Scene::add_material(NCF1 *p)
{
	std::string type = p->get(XT_CFGPROTO_PROP_TYPE);

	Material *mat = new Material();

	std::string colr, colg, colb;

	if (!type.compare(XT_CFGPROTO_VAL_LAMBERT))
	{
		mat->type = MATERIAL_LAMBERT;
	}
	else if (!type.compare(XT_CFGPROTO_VAL_PHONG))
	{
		mat->type = MATERIAL_PHONG;
	}
	else if (!type.compare(XT_CFGPROTO_VAL_BLINNPHONG))
	{
		mat->type = MATERIAL_BLINNPHONG;
	}
	else
	{
		std::cout << "Warning: Unsupported material " << p->node() << ". Skipping...\n";
		delete mat;
		return 1;
	}

	// common properties
	if (mat)
	{
		// specular intensity
		colr = p->group(XT_CFGPROTO_PROP_SPECULAR)->get(XT_CFGPROTO_PROP_COLOR_R);
		colg = p->group(XT_CFGPROTO_PROP_SPECULAR)->get(XT_CFGPROTO_PROP_COLOR_G);
		colb = p->group(XT_CFGPROTO_PROP_SPECULAR)->get(XT_CFGPROTO_PROP_COLOR_B);

		mat->specular = 
			Vector3(nstring_to_double(colr), 
					nstring_to_double(colg), 
					nstring_to_double(colb));

		// specular constant
		mat->kspec = nstring_to_double(p->get(XT_CFGPROTO_PROP_KSPEC));
		// diffuse constant
		mat->kdiff = nstring_to_double(p->get(XT_CFGPROTO_PROP_KDIFF));
		// specular exponential
		mat->ksexp = nstring_to_double(p->get(XT_CFGPROTO_PROP_KSEXP));

		// diffuse intensity
		colr = p->group(XT_CFGPROTO_PROP_DIFFUSE)->get(XT_CFGPROTO_PROP_COLOR_R);
		colg = p->group(XT_CFGPROTO_PROP_DIFFUSE)->get(XT_CFGPROTO_PROP_COLOR_G);
		colb = p->group(XT_CFGPROTO_PROP_DIFFUSE)->get(XT_CFGPROTO_PROP_COLOR_B);
		
		mat->diffuse = 
			Vector3(nstring_to_double(colr), 
					nstring_to_double(colg), 
					nstring_to_double(colb));
	
		// reflectance
		std::string refl = p->get(XT_CFGPROTO_PROP_REFLECTANCE);
		mat->reflectance = nstring_to_double(refl);
		
		if (mat->reflectance > 1.0)
			mat->reflectance = 1.0;
		else if (mat->reflectance < 0.0)
			mat->reflectance = 0.0;

		// transparency
		std::string transp = p->get(XT_CFGPROTO_PROP_TRANSPARENCY);
		mat->transparency = nstring_to_double(transp);

		if (mat->transparency > 1.0)
			mat->transparency = 1.0;
		else if (mat->transparency < 0.0)
			mat->transparency = 0.0;
		
		// index of refraction
		std::string ior = p->get(XT_CFGPROTO_PROP_IOR);
		mat->ior = nstring_to_double(ior);
	}

	material[p->node()] = mat;

	return 0;
}

unsigned int Scene::add_object(NCF1 *p)
{
	std::string comp;
	Object *tobj = new Object;

	// Set the geometry
	comp = p->get(XT_CFGPROTO_PROP_GEOMETRY);
	if(geometry.find(comp)!=geometry.end())
	{
		tobj->geometry = comp;
	}
	else
	{
		std::cerr 
			<< "Warning: At object: " << p->node() 
			<<  " geometry " << comp << " does not exist. Skipping.\n";
		delete tobj;
		return 1;
	}

	// Set the material
	comp = p->get(XT_CFGPROTO_PROP_MATERIAL);
	if(material.find(comp)!=material.end())
	{
		tobj->material = comp;
	}
	else
	{
		std::cerr
			<< "Warning: At object: " << p->node() 
			<<  " material " << comp << " does not exist. Skipping.\n";
		delete tobj;
		return 1;
	}
	object[p->node()] = tobj;

	return 0;
}

bool Scene::intersection(const Ray &ray, IntInfo &info, std::string &obj, bool lights)
{
	IntInfo test, res;

	std::map<std::string, Object *>::iterator it;
	for (it = object.begin(); it != object.end(); it++)
	{
		// test all the objects and find the closest intersection
		if((geometry[(*it).second->geometry.c_str()])->intersection(ray, &test))
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

	// check for light intersections
	if (lights)
	{
		std::map<std::string, Light *>::iterator itl;
		for (itl = light.begin(); itl != light.end(); itl++)
		{
			Sphere light;
			light.origin = (*itl).second->position;
			light.radius = 10;
			light.calc_aabb();
			if (light.intersection(ray, &test))
			{
				if(res.t > test.t)
				{
					obj.clear();	
				}
			}
		}
	}

	// copy result to info
	memcpy(&info, &res, sizeof(info));

	return info.t != NM_INFINITY ? true : false;
}

unsigned int Scene::apply_modifier(const char *reg)
{
	std::string m(reg);
	std::cout << "Applying modifier: " << m ;

	// Check if there is actually an asignment
	if(m.find_last_of(':') == std::string::npos)
	{
		std::cout << "Warning: No value assignment. Skipping..\n";
		return 1;
	}

	NCF1 *node = &data;
	std::string nleft, nright;

	while((m.find_first_of('.') != std::string::npos) && (m.find_first_of(':') > m.find_first_of('.')))
	{
		nstring_split(m, nleft, nright, '.');
		m = nright;

		// move to node
		node = node->group(nleft.c_str());
	}

	nstring_split(m, nleft, nright, ':');

	std::cout<< " [" << node->get(nleft.c_str()) << "]\n";
	node->set(nleft.c_str(), nright.c_str());

	return 0;
}
