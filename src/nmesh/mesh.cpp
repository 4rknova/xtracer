#include <nmath/precision.h>
#include <nmath/vector.h>

#include <nmath/intinfo.h>
#include "defs.h"
#include "vertex.h"
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

	const Buffer<NMesh::vertex_t> &vref = vertices_ro();

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

	//m_octree.max_items_per_node(Environment::handle().octree_max_items_per_node());
	//m_octree.max_depth(Environment::handle().octree_max_depth());

	m_octree.max_items_per_node(10);
	m_octree.max_depth(10);
	m_octree.build();
}
