#ifndef XTCORE_MESH_H_INCLUDED
#define XTCORE_MESH_H_INCLUDED

#include <nmesh/obj.h>
#include "math/surface.h"
#include "math/triangle.h"
#include "math/ray.h"
#include "octree.h"
#include "structs.h"

#define LIMITS_MAX_ITEMS_PER_NODE 10
#define LIMITS_MAX_DEPTH          5

using NMath::Vector3f;
using nmesh::shape_t;
using nmesh::attrib_t;
using nmesh::object_t;

namespace xtcore {
    namespace surface {

class Mesh: public xtcore::asset::ISurface
{
	public:
 	 Mesh();
	~Mesh();

	bool intersection(const Ray &ray, HitRecord *i_info) const;
    NMath::scalar_t distance(NMath::Vector3f p) const;
	void calc_aabb();
    void build_octree(shape_t &shape, attrib_t &attributes);
    void build_octree(object_t &object);

    Vector3f point_sample() const;
    Ray ray_sample() const;

	private:
	Octree<Triangle> m_octree;
};

    } /* namespace surface */
} /* namespace xtcore */

#endif /* XTCORE_MESH_H_INCLUDED */
