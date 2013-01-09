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

#include <nmath/defs.h>
#include <nmath/precision.h>
#include <nmath/intinfo.h>
#include <nmath/vector.h>
#include <nmesh/vertex.h>
#include "argparse.hpp"
#include "mesh.hpp"

Mesh::Mesh()
	: Geometry(NMath::GEOMETRY_MESH)
{}

Mesh::~Mesh()
{}

bool Mesh::intersection(const Ray &ray, IntInfo* i_info) const
{
	if(!aabb.intersection(ray)) {
		return false;
	}

	if (m_octree.intersection(ray, i_info) != NULL) {
		return true;
	}

	return false;
}

void Mesh::calc_aabb()
{
	aabb.min = Vector3f(INFINITY, INFINITY, INFINITY);
	aabb.max = Vector3f(-INFINITY, -INFINITY, -INFINITY);

	const NMesh::Buffer<NMesh::vertex_t> &vref = vertices_ro();

	for (unsigned int i = 0; i < vref.count(); ++i) {
		Vector3f v = Vector3f(vref[i].px, vref[i].py, vref[i].pz);

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

void Mesh::build_octree()
{
	for (unsigned int i = 0; i < indices_ro().count(); i+=3) {
		Triangle p;
		IntInfo inf;

		p.v[0] = Vector3f(vertices_ro()[indices_ro()[i  ]].px,
						  vertices_ro()[indices_ro()[i  ]].py,
						  vertices_ro()[indices_ro()[i  ]].pz);

		p.v[1] = Vector3f(vertices_ro()[indices_ro()[i+1]].px,
						  vertices_ro()[indices_ro()[i+1]].py,
						  vertices_ro()[indices_ro()[i+1]].pz);

		p.v[2] = Vector3f(vertices_ro()[indices_ro()[i+2]].px,
						  vertices_ro()[indices_ro()[i+2]].py,
						  vertices_ro()[indices_ro()[i+2]].pz);

		p.n[0] = Vector3f(vertices_ro()[indices_ro()[i  ]].nx,
						  vertices_ro()[indices_ro()[i  ]].ny,
						  vertices_ro()[indices_ro()[i  ]].nz);

		p.n[1] = Vector3f(vertices_ro()[indices_ro()[i+1]].nx,
						  vertices_ro()[indices_ro()[i+1]].ny,
						  vertices_ro()[indices_ro()[i+1]].nz);

		p.n[2] = Vector3f(vertices_ro()[indices_ro()[i+2]].nx,
						  vertices_ro()[indices_ro()[i+2]].ny,
						  vertices_ro()[indices_ro()[i+2]].nz);

		p.tc[0] = Vector3f(vertices_ro()[indices_ro()[i  ]].u,
						   vertices_ro()[indices_ro()[i  ]].v);

		p.tc[1] = Vector3f(vertices_ro()[indices_ro()[i+1]].u,
						   vertices_ro()[indices_ro()[i+1]].v);

		p.tc[2] = Vector3f(vertices_ro()[indices_ro()[i+2]].u,
						   vertices_ro()[indices_ro()[i+2]].v);

		p.calc_aabb();

		m_octree.add(p.aabb, p);
	}

	m_octree.max_items_per_node(Environment::handle().octree_max_items_per_node());
	m_octree.max_depth(Environment::handle().octree_max_depth());
	m_octree.build();
}
