#ifndef XTCORE_MESH_H_INCLUDED
#define XTCORE_MESH_H_INCLUDED

#include <cstdint>
#include <vector>

#include <nmesh/obj.h>
#include "math/surface.h"
#include "math/triangle.h"
#include "math/ray.h"
#include "structs.h"

#define LIMITS_MAX_ITEMS_PER_LEAF 8
#define LIMITS_MAX_BVH_DEPTH      32

using nmath::Vector3f;
using nmesh::shape_t;
using nmesh::attrib_t;
using nmesh::object_t;

namespace xtcore {
struct mesh_bvh_stats_t
{
    size_t triangles;
    size_t bvh_nodes;
    size_t bvh_leaves;
    double build_ms;
    unsigned long long build_count;
    unsigned long long rays_tested;
    unsigned long long rays_hit;
    unsigned long long nodes_visited;
    unsigned long long triangle_tests;
};

mesh_bvh_stats_t get_last_mesh_bvh_stats();
void reset_mesh_bvh_stats();

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
    void collect_bvh_aabbs(std::vector<AABB3> &out) const;

    Vector3f point_sample() const;
    Ray ray_sample() const;
    Vector3f emitter_position() const;

	private:
    struct bvh_node_t {
        AABB3 aabb;
        uint32_t left;
        uint32_t right;
        uint32_t first;
        uint32_t count;
    };

    void build_bvh();
    uint32_t build_bvh_node(uint32_t first, uint32_t count, uint32_t depth);

    uv_projection_t m_uv_projection;
    std::vector<xtcore::surface::Triangle> m_triangles;
    std::vector<uint32_t> m_bvh_indices;
    std::vector<bvh_node_t> m_bvh_nodes;
};

    } /* namespace surface */
} /* namespace xtcore */

#endif /* XTCORE_MESH_H_INCLUDED */
