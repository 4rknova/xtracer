#ifndef LIBNMESH_MESH_H_INCLUDED
#define LIBNMESH_MESH_H_INCLUDED

#include <nmath/ray.h>
#include <nmath/geometry.h>
#include <nmath/triangle.h>
#include <nmath/octree.hpp>
#include "structs.h"

#define LIMITS_MAX_ITEMS_PER_NODE 10
#define LIMITS_MAX_DEPTH          10

using NMath::Ray;
using NMath::Geometry;

namespace NMesh {

class MTriangle : public NMath::Triangle
{
	public:
		virtual ~MTriangle();
		int material_id;
};

class Mesh: public Geometry
{
	public:
		Mesh();
		~Mesh();

		bool intersection(const Ray &ray, IntInfo* i_info) const;
		void calc_aabb();
        void build_octree(object_t &object);

        NMath::Vector3f point_sample() const;
        NMath::Ray ray_sample() const;

	private:
		Octree<MTriangle> m_octree;
};

} /* namespace NMesh */

#endif /* LIBNMESH_MESH_H_INCLUDED */
