/*

    This file is part of xtracer. 

    mesh.cc
    Mesh

    Copyright (C) 2008, 2010, 2011
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

#include "mesh.hpp"

#include <nmath/defs.h>
#include <nmath/precision.h>
#include <nmath/vector.h>
#include <nmath/intinfo.h>

Mesh::Mesh()
	: Geometry(GEOMETRY_MESH)
{}

Mesh::~Mesh()
{}

#include <nmath/triangle.h>

bool Mesh::intersection(const Ray &ray, IntInfo* i_info) const
{
	if(!aabb.intersection(ray))
	{
		return false;
	}

	bool found = false;

	IntInfo nearest;
	nearest.t = INFINITY;

	Triangle p;
	Vector3 normal; // Used for phong shading

	for(size_t i = 0; i < faces.size(); i++)
	{
		IntInfo inf;

		p.v[0] = vertices[faces[i].v[0]].position;
		p.v[1] = vertices[faces[i].v[1]].position;
		p.v[2] = vertices[faces[i].v[2]].position;
		p.n[0] = vertices[faces[i].v[0]].normal;
		p.n[1] = vertices[faces[i].v[1]].normal;
		p.n[2] = vertices[faces[i].v[2]].normal;

		p.calc_aabb();

		if(p.intersection(ray, &inf) && inf.t < nearest.t) 
		{
			nearest = inf;
			found = true;
		}
	}

	if(!found) 
	{
		return false;
	}

	if(i_info)
	{

		*i_info = nearest;
		i_info->geometry = this;
	}

	return true;
}

void Mesh::calc_aabb()
{
	aabb.min = Vector3(NM_INFINITY, NM_INFINITY, NM_INFINITY);
	aabb.max = Vector3(-NM_INFINITY, -NM_INFINITY, -NM_INFINITY);
		
	for(size_t i = 0; i < faces.size(); i++) 
	{
		for(int j = 0; j < 3; j++) 
		{
			Vector3 v = vertices[faces[i].v[j]].position; 

			// min
			if(v.x < aabb.min.x) aabb.min.x = v.x;
			if(v.y < aabb.min.y) aabb.min.y = v.y;
			if(v.z < aabb.min.z) aabb.min.z = v.z;

			// max
			if(v.x > aabb.max.x) aabb.max.x = v.x;
			if(v.y > aabb.max.y) aabb.max.y = v.y;
			if(v.z > aabb.max.z) aabb.max.z = v.z;
		}
	}
}

void Mesh::calc_vertex_normals()
{
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		// Calculate the face normal
		Vector3 v1 = vertices[faces[i].v[2]].position 
						- vertices[faces[i].v[0]].position;
		Vector3 v2 = vertices[faces[i].v[1]].position
						- vertices[faces[i].v[0]].position;
		Vector3 fnormal = (cross(v1, v2)).normalized();

		// Modify the vertex normal
		vertices[faces[i].v[0]].normal += fnormal/3;
		vertices[faces[i].v[0]].normal.normalize();

		vertices[faces[i].v[1]].normal += fnormal/3;
		vertices[faces[i].v[1]].normal.normalize();

		vertices[faces[i].v[2]].normal += fnormal/3;
		vertices[faces[i].v[2]].normal.normalize();
	}
}
