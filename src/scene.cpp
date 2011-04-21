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
		for(std::map<std::string, Light *>::iterator it = light.begin(); it != light.end(); it++)
			delete (*it).second;
	}
/*
	// Release the materials
	if(!material.empty())
	{
		std::cout << "Releasing the materials..\n";
		for(std::map<std::string, Material *>::iterator it = material.begin(); it != material.end(); it++)
			delete (*it).second;
	} 
*/
	// Release the object
	if(!object.empty())
	{
		std::cout << "Releasing the objects..\n";
		for(std::map<std::string, Object *>::iterator it = object.begin(); it != object.end(); it++)
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

	// Set the scene ambient color
	set_ambient();

	// Set the camera
	set_camera(NULL);

	// Populate the light sources
	count = data.group(XT_CFGPROTO_NODE_LIGHT)->count_groups();
	if (count)
	{
		for (unsigned int i = 1; i<= count; i++)
		{
			NCFGParser *lnode = data.group(XT_CFGPROTO_NODE_LIGHT)->group(i);
			add_light(lnode);
		}
	}
	else
	{
		std::cout << "Warning: There are no lights in this scene\n";
	}

	// Populate the materials
	count = data.group(XT_CFGPROTO_NODE_MATERIAL)->count_groups();
	if(count)
	{
		for (unsigned int i = 1; i <= count; i++)
		{
			NCFGParser *lnode = data.group(XT_CFGPROTO_NODE_MATERIAL)->group(i);
			add_material(lnode);
		}
	}
	
	// Populate the object
	count = data.group(XT_CFGPROTO_NODE_GEOMETRY)->count_groups();
	if (count)
	{
		for (unsigned int i = 1; i<= count; i++)
		{
			NCFGParser *lnode = data.group(XT_CFGPROTO_NODE_GEOMETRY)->group(i);
			add_object(lnode);
		}
	}

	return 0;
}

unsigned int Scene::analyze()
{
	std::cout << "Analyzing scene data..\n";
	/* Print statistics */
	std::string name = data.get(XT_CFGPROTO_PROP_NAME);
	std::string info = data.get(XT_CFGPROTO_PROP_DESCRIPTION);

	if(!name.empty())
		std::cout 
			<< "[ " << data.get(XT_CFGPROTO_PROP_NAME) << " ]";
	else
		std::cout
			<< "[ scene ]";
	
	std::cout << "\n";

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

	unsigned int n = nodes.size();
	std::string pad = "   ";

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

	//std::cout << "Setting ambient color: " << ambient << "\n";

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

unsigned int Scene::add_object(NCFGParser *p)
{
	std::string type = p->get(XT_CFGPROTO_PROP_TYPE);

	std::cout << "Adding " << type << ": " << p->node() << "\n";

	// determine the geometry type

	Object *tobj = NULL;

	// sphere
	if (!type.compare(XT_CFGPROTO_VAL_SPHERE))
	{
		tobj = new Object(GEOMETRY_SPHERE);

		std::string x, y, z, r;

		x = p->group(XT_CFGPROTO_PROP_POSITION)->get(XT_CFGPROTO_PROP_COORD_X);
		y = p->group(XT_CFGPROTO_PROP_POSITION)->get(XT_CFGPROTO_PROP_COORD_Y);
		z = p->group(XT_CFGPROTO_PROP_POSITION)->get(XT_CFGPROTO_PROP_COORD_Z);
		r = p->get(XT_CFGPROTO_PROP_RADIUS);

		// set the position
		((Sphere *)tobj)->origin = Vector3(
			nstring_to_double(x), 
			nstring_to_double(y), 
			nstring_to_double(z));

		// set the radius
		((Sphere *)tobj)->radius = nstring_to_double(r);
	}
	// plane
	else if (!type.compare(XT_CFGPROTO_VAL_PLANE))
	{
		tobj = new Object(GEOMETRY_PLANE);

		std::string x, y, z, d;

		x = p->group(XT_CFGPROTO_PROP_NORMAL)->get(XT_CFGPROTO_PROP_COORD_X);
		y = p->group(XT_CFGPROTO_PROP_NORMAL)->get(XT_CFGPROTO_PROP_COORD_Y);
		z = p->group(XT_CFGPROTO_PROP_NORMAL)->get(XT_CFGPROTO_PROP_COORD_Z);
		d = p->get(XT_CFGPROTO_PROP_DISTANCE);

		// set the normal
		((Plane *)tobj)->normal = Vector3(
			nstring_to_double(x), 
			nstring_to_double(y), 
			nstring_to_double(z));

		// set the distance
		((Plane *)tobj)->distance = nstring_to_double(d);
	}
	// unknown
	else
	{
		std::cout << "Warning: Unknown type: " << type <<". Ingoring..\n";
		return 1;
	}

	// push the object to the map
	object[p->node()] = tobj;

	// Set the material
	std::string mat = p->get(XT_CFGPROTO_PROP_MATERIAL);
	if(material.find(mat)!=material.end())
	{
		tobj->material = mat;
	}
	else
	{
		std::cout << "Warning: material: " << mat << " does not exist.\n";
	}

	return 0;
} 

unsigned int Scene::add_material(NCFGParser *p)
{

	std::cout << "Adding " << p->get(XT_CFGPROTO_PROP_TYPE) << " material: " << p->node() << "\n";
/*
	if (!type.compare(XT_CFGPROTO_VAL_LAMBERT))
	{
		std::string colr = p->group(XT_CFGPROTO_PROP_DIFFUSE)->get(XT_CFGPROTO_PROP_COLOR_R);
		std::string colg = p->group(XT_CFGPROTO_PROP_DIFFUSE)->get(XT_CFGPROTO_PROP_COLOR_G);
		std::string colb = p->group(XT_CFGPROTO_PROP_DIFFUSE)->get(XT_CFGPROTO_PROP_COLOR_B);

		std::string refl = p->get(XT_CFGPROTO_PROP_REFLECTANCE);
		
		Material *tmat = new Material(MATERIAL_TYPE_LAMBERT);
	
		((MatLambert *)(tmat->get()))->diffuse = 
			Vector3(nstring_to_double(colr), 
					nstring_to_double(colg), 
					nstring_to_double(colb));

		((MatLambert *)(tmat->get()))->reflectance = nstring_to_double(refl);

		material[p->node()] = tmat;
	}
	else
	{
		std::cout << "Warning: Unsupported model. Skipping...\n";
	}
*/
	return 0;
}
