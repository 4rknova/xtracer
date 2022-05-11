#ifndef XTCORE_BVH_H_INCLUDED
#define XTCORE_BVH_H_INCLUDED

#include <nmath/vector.h>
#include <nmesh/structs.h>

namespace xtcore {
    namespace spatial {
        namespace bvh {
/*
using nmath::Vector3f;
using nmesh::object_t;

typedef struct {
    Vector3f aabb_min;
    Vector3f aabb_max;

    uint child_left;
    uint prim_first;
    uint prim_count;

    bool is_leaf();
} node_t;

template <typename T>
typedef struct {
    node_t *nodes;
    uint node_idx_root;
    uint nodes_used;

    std::vector<T> data;
    std::vector<uint> indices;
} bvh_t;

void build(bvh_t &bvh, object_t &object);

void convert(ISurface)
*/
        } /* namespace bvh */
    } /* namespace spatial */
} /* namespace xtcore */

#endif /* XTCORE_BVH_H_INCLUDED */

