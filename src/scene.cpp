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

#include <string>
#include <iostream>
#include <iomanip>

#include <nmath/vector.h>
#include <nmath/geometry.h>
#include <nparse/parseutils.hpp>

#include "scene.hpp"
#include "proto.h"

Scene::Scene(const char *filepath)
	: 
		camera(new Camera()),
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
}

unsigned int Scene::init()
{
	std::cout << "Initiating the scene..\n";		

	// Load the scene file and parse it
	int status = data.parse();
	if(status)
	{
		std::cerr << "Error: Failed to load the scene file.\n";
		return 1;
	}

	// Start populating the lists
	unsigned int count = 0;

	set_ambient();			// Set the scene ambient color
	set_camera(NULL);		// Set the camera

	std::list<std::string> sections;
	sections.push_back(XT_CFGPROTO_NODE_LIGHT);
	sections.push_back(XT_CFGPROTO_NODE_MATERIAL);
	sections.push_back(XT_CFGPROTO_NODE_GEOMETRY);
	sections.push_back(XT_CFGPROTO_NODE_OBJECT);

	std::list<std::string>::iterator it;

	for (it = sections.begin(); it != sections.end(); it++)
	{
		// Populate the groups
		count = data.group((*it).c_str())->count_groups();
		if (count)
		{
			for (unsigned int i = 1; i<= count; i++)
			{
				NCFGParser *lnode = data.group((*it).c_str())->group(i);

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
	std::string r, g, b;
	r = data.group(XT_CFGPROTO_PROP_AMBIENT)->get(XT_CFGPROTO_PROP_COLOR_R);
	g = data.group(XT_CFGPROTO_PROP_AMBIENT)->get(XT_CFGPROTO_PROP_COLOR_G);
	b = data.group(XT_CFGPROTO_PROP_AMBIENT)->get(XT_CFGPROTO_PROP_COLOR_B);

	ambient = Vector3(nstring_to_double(r), nstring_to_double(g), nstring_to_double(b));

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

	// cleanup the previous camera ( I allocate a generic camera in the constructor )
	delete camera;

	// create the camera
	camera = new Camera(pos, targ, up, fov);

	return 0;
}

unsigned int Scene::add_light(NCFGParser *p)
{
	std::cout << "Adding light: " << p->node() << "\n";

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

unsigned int Scene::add_geometry(NCFGParser *p)
{
	std::string type = p->get(XT_CFGPROTO_PROP_TYPE);

	std::cout << "Adding geometry: " << p->node() << " [ " << type << " ]\n";

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
	// unknown
	else
	{
		std::cout << "Warning: Unknown type: " << type <<". Ingoring..\n";
		return 1;
	}

	geo->calc_bbox();
	geometry[p->node()] = geo;
	return 0;
}

#include "matlambert.hpp"
#include "matphong.hpp"

unsigned int Scene::add_material(NCFGParser *p)
{
	std::string type = p->get(XT_CFGPROTO_PROP_TYPE);

	std::cout << "Adding material: " << p->node() << " [ " << type << " ]\n";

	std::string refl = p->get(XT_CFGPROTO_PROP_REFLECTANCE);

	Material *mat = NULL;

	std::string colr, colg, colb;

	if (!type.compare(XT_CFGPROTO_VAL_LAMBERT))
	{
		mat = new MatLambert();

		// diffuse intensity
		colr = p->group(XT_CFGPROTO_PROP_DIFFUSE)->get(XT_CFGPROTO_PROP_COLOR_R);
		colg = p->group(XT_CFGPROTO_PROP_DIFFUSE)->get(XT_CFGPROTO_PROP_COLOR_G);
		colb = p->group(XT_CFGPROTO_PROP_DIFFUSE)->get(XT_CFGPROTO_PROP_COLOR_B);
		
		((MatLambert *)mat)->diffuse = 
			Vector3(nstring_to_double(colr), 
					nstring_to_double(colg), 
					nstring_to_double(colb));
	}
	else if (!type.compare(XT_CFGPROTO_VAL_PHONG))
	{
		mat = new MatPhong();

		colr = p->group(XT_CFGPROTO_PROP_DIFFUSE)->get(XT_CFGPROTO_PROP_COLOR_R);
		colg = p->group(XT_CFGPROTO_PROP_DIFFUSE)->get(XT_CFGPROTO_PROP_COLOR_G);
		colb = p->group(XT_CFGPROTO_PROP_DIFFUSE)->get(XT_CFGPROTO_PROP_COLOR_B);

		// diffuse intensity
		((MatPhong *)mat)->diffuse = 
			Vector3(nstring_to_double(colr), 
					nstring_to_double(colg), 
					nstring_to_double(colb));

		// specular intensity
		colr = p->group(XT_CFGPROTO_PROP_SPECULAR)->get(XT_CFGPROTO_PROP_COLOR_R);
		colg = p->group(XT_CFGPROTO_PROP_SPECULAR)->get(XT_CFGPROTO_PROP_COLOR_G);
		colb = p->group(XT_CFGPROTO_PROP_SPECULAR)->get(XT_CFGPROTO_PROP_COLOR_B);

		// diffuse constant
		((MatPhong *)mat)->specular = 
			Vector3(nstring_to_double(colr), 
					nstring_to_double(colg), 
					nstring_to_double(colb));

		// specular constant
		((MatPhong *)mat)->kspec = nstring_to_double(p->get(XT_CFGPROTO_PROP_KSPEC));
		// diffuse constant
		((MatPhong *)mat)->kdiff = nstring_to_double(p->get(XT_CFGPROTO_PROP_KDIFF));
		// specular exponential
		((MatPhong *)mat)->ksexp = nstring_to_double(p->get(XT_CFGPROTO_PROP_KSEXP));
	}
	else
	{
		std::cout << "Warning: Unsupported material. Skipping...\n";
		return 1;
	}

	mat->reflectance = nstring_to_double(refl);
	material[p->node()] = mat;

	return 0;
}

unsigned int Scene::add_object(NCFGParser *p)
{
	std::cout << "Adding object: " << p->node() << "\n";
	
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
		std::cerr << "Warning: geometry " << comp << " does not exist. Skipping.\n";
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
		std::cerr << "Warning: material " << comp << " does not exist. Skipping.\n";
		delete tobj;
		return 1;
	}
	object[p->node()] = tobj;

	return 0;
}

bool Scene::intersection(const Ray &ray, IntInfo &info, std::string &obj)
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

	// copy result to info
	memcpy(&info, &res, sizeof(info));

	return info.t != NM_INFINITY ? true : false;
}
