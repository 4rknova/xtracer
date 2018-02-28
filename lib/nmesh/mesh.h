#ifndef LIBNMESH_MESH_H_INCLUDED
#define LIBNMESH_MESH_H_INCLUDED

#include <nmath/ray.h>
#include <nmath/geometry.h>
#include <nmath/triangle.h>
#include <nsps/octree.h>
#include "structs.h"

#define LIMITS_MAX_ITEMS_PER_NODE 10
#define LIMITS_MAX_DEPTH          5

namespace nmesh {

class Mesh: public NMath::Geometry
{
	public:
		Mesh();
		~Mesh();

		bool intersection(const Ray &ray, IntInfo *i_info) const;
		void calc_aabb();
        void build_octree(shape_t &shape, attrib_t &attributes);
        void build_octree(object_t &object);

        NMath::Vector3f point_sample() const;
        NMath::Ray ray_sample() const;

	private:
		nsps::Octree<NMath::Triangle> m_octree;
};

} /* namespace nmesh */

#endif /* LIBNMESH_MESH_H_INCLUDED */
