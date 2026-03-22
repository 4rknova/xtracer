#ifndef XTCORE_MESH_H_INCLUDED
#define XTCORE_MESH_H_INCLUDED

#include <nmesh/obj.h>
#include "math/surface.h"
#include "math/triangle.h"
#include "math/ray.h"
#include "octree.h"
#include "structs.h"

#define LIMITS_MAX_ITEMS_PER_NODE 10
#define LIMITS_MAX_DEPTH          10

using nmath::Vector3f;
using nmesh::shape_t;
using nmesh::attrib_t;
using nmesh::object_t;

namespace xtcore {
    namespace surface {

class Mesh: public xtcore::asset::ISurface
{
	public:
    enum uv_projection_t {
        UV_PROJECTION_SOURCE = 0,
        UV_PROJECTION_CYLINDRICAL_Y
    };

 	 Mesh();
	~Mesh();

	bool intersection(const Ray &ray, hit_record_t *i_hit_record) const;
    nmath::scalar_t distance(nmath::Vector3f p) const;
	void calc_aabb();
    void build_octree(shape_t &shape, attrib_t &attributes);
    void build_octree(object_t &object);
    void set_uv_projection(uv_projection_t projection);
    uv_projection_t uv_projection() const;
    const std::vector<xtcore::surface::Triangle> &triangles() const;

    Vector3f point_sample() const;
    Ray ray_sample() const;
    Vector3f emitter_position() const;

	private:
    uv_projection_t m_uv_projection;
	Octree<Triangle> m_octree;
    std::vector<xtcore::surface::Triangle> m_triangles;
};

    } /* namespace surface */
} /* namespace xtcore */

#endif /* XTCORE_MESH_H_INCLUDED */
