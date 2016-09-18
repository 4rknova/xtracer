#include <nmath/precision.h>
#include <nmath/vector.h>
#include <nmath/intinfo.h>
#include "mesh.h"

namespace NMesh {

MTriangle::~MTriangle()
{}

Mesh::Mesh()
	: Geometry(NMath::GEOMETRY_MESH)
{}

Mesh::~Mesh()
{}

bool Mesh::intersection(const Ray &ray, IntInfo* i_info) const
{
	return m_octree.intersection(ray, i_info) != NULL;
}

void Mesh::calc_aabb()
{}

void Mesh::build_octree(object_t &object)
{
    std::vector<shape_t>::iterator it = object.shapes.begin();
    std::vector<shape_t>::iterator et = object.shapes.end();

    for (; it != et; ++it) {
     	for (size_t i = 0; i < (*it).mesh.indices.size()/3; ++i) {

            int a_v = (*it).mesh.indices[3*i  ].v;
            int b_v = (*it).mesh.indices[3*i+1].v;
            int c_v = (*it).mesh.indices[3*i+2].v;
            int a_n = (*it).mesh.indices[3*i  ].n;
            int b_n = (*it).mesh.indices[3*i+1].n;
            int c_n = (*it).mesh.indices[3*i+2].n;
            int a_t = (*it).mesh.indices[3*i  ].uv;
            int b_t = (*it).mesh.indices[3*i+1].uv;
            int c_t = (*it).mesh.indices[3*i+2].uv;

            attrib_t *d = &(object.attributes);

    		MTriangle p;

            p.material_id = (*it).mesh.materials[i];

            if (a_v != -1) p.v[0]  = NMath::Vector3f(d->v[3*a_v], d->v[3*a_v+1], d->v[3*a_v+2]);
		    if (b_v != -1) p.v[1]  = NMath::Vector3f(d->v[3*b_v], d->v[3*b_v+1], d->v[3*b_v+2]);
		    if (c_v != -1) p.v[2]  = NMath::Vector3f(d->v[3*c_v], d->v[3*c_v+1], d->v[3*c_v+2]);
		    if (a_n != -1) p.n[0]  = NMath::Vector3f(d->n[3*a_n], d->n[3*a_n+1], d->n[3*a_n+2]);
		    if (b_n != -1) p.n[1]  = NMath::Vector3f(d->n[3*b_n], d->n[3*b_n+1], d->n[3*b_n+2]);
		    if (c_n != -1) p.n[2]  = NMath::Vector3f(d->n[3*c_n], d->n[3*c_n+1], d->n[3*c_n+2]);

		    if (a_t != -1) p.tc[0] = NMath::Vector3f(d->uv[2*a_t], d->uv[2*a_t+1], 0);
		    if (b_t != -1) p.tc[1] = NMath::Vector3f(d->uv[2*b_t], d->uv[2*b_t+1], 0);
		    if (c_t != -1) p.tc[2] = NMath::Vector3f(d->uv[2*c_t], d->uv[2*c_t+1], 0);

		    p.calc_aabb();
    		m_octree.add(p.aabb, p);
        }
    }
	m_octree.max_items_per_node(LIMITS_MAX_ITEMS_PER_NODE);
	m_octree.max_depth(LIMITS_MAX_DEPTH);
	m_octree.build();
}

} /* namespace NMesh */
