#include <types.h>
#include <nmath/mutil.h>
#include "bvh.h"
#include "math/triangle.h"

/* References:
** 1. https://jacco.ompf2.com/2022/04/13/how-to-build-a-bvh-part-1-basics/
*/

namespace xtcore {
    namespace spatial {
        namespace bvh {
/*
using nmath::min;
using nmath::max;
using nmath::Vector3f;
using xtcore::surface::Triangle;
using nmesh::mesh_t;
using nmesh::attrib_t;

bool node_t::is_leaf() {  return prim_count > 0; }

Vector3f min_vec3(Vector3f a, Vector3f b) { return Vector3f(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z)); }
Vector3f max_vec3(Vector3f a, Vector3f b) { return Vector3f(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z)); }

void update_bounds(node_t &node, mesh_t &mesh, attrib_t &attrib)
{
    node.aabb_min = Vector3f( EPSILON);
    node.aabb_max = Vector3f(-EPSILON);

    for (uint first = node.prim_first, i = 0; i < node.prim_count; ++i) {
        uint idx_offset = 3 * (first + i);                // The index array offset for the first triangle
        uint vrt_offset = 9 * mesh.indices[idx_offset].v; // The vertex array offset for the first triangle

        Vector3f v0 = Vector3f(attrib.v[vrt_offset + 0], attrib.v[vrt_offset + 1], attrib.v[vrt_offset + 2]);
        Vector3f v1 = Vector3f(attrib.v[vrt_offset + 3], attrib.v[vrt_offset + 4], attrib.v[vrt_offset + 5]);
        Vector3f v2 = Vector3f(attrib.v[vrt_offset + 6], attrib.v[vrt_offset + 7], attrib.v[vrt_offset + 8]);

        node.aabb_min = min_vec3(node.aabb_min, v0);
        node.aabb_min = min_vec3(node.aabb_min, v1);
        node.aabb_min = min_vec3(node.aabb_min, v2);
        node.aabb_max = max_vec3(node.aabb_max, v0);
        node.aabb_max = max_vec3(node.aabb_max, v1);
        node.aabb_max = max_vec3(node.aabb_max, v2);
    }
}

void subdivide(bvh_t &bvh, uint idx)
{
    node_t node = bvh.nodes[idx];

    // Terminate recursion if the node contains 2 or less primitives.
    if (node.prim_count <= 2) return;

    // Step 1. Split plane axis and position.
    // We will split the AABB along its longest axis. There are better ways to do this, but for now it's fine.
    Vector3f extent = node.aabb_max - node.aabb_min;
    int axis = 0;
    if (extent.y > extent.x) axis = 1;
    if (extent.z > extent[axis]) axis = 2;
    float splitPos = node.aabb_min[axis] + extent[axis] * 0.5f;

    // Step 2. Split the group in two halves.
    // The list is split in place. We walk the list of primitives, and swap each primitive that is not on the left of
    // the plane with a primitive at the end of the list. This is functionally equivalent to a QuickSort partition.
    uint i = node.prim_first * 3;
    uint j = (i + node.prim_count - 1) * 3;

    while (i <= j)
    {
        if (tri[triIdx[i]].centroid[axis] < splitPos)
            i++;
        else
            swap( triIdx[i], triIdx[j--] );
    }
    // abort split if one of the sides is empty
    int leftCount = i - node.firstTriIdx;
    if (leftCount == 0 || leftCount == node.triCount) return;


    // Step 3. Creating child nodes for each half
    // Using the outcome of the partition loop we construct two child nodes. The first child node contains the
    // primitives at the start of the array; the second child node contains the primitives starting at i. The number of
    // primitives on the left is thus i - node.firstPrim; the right child has the remaining primitives.
    int leftChildIdx = nodesUsed++;
    int rightChildIdx = nodesUsed++;
    node.leftNode = leftChildIdx;
    bvhNode[leftChildIdx].firstTriIdx = node.firstTriIdx;
    bvhNode[leftChildIdx].triCount = leftCount;
    bvhNode[rightChildIdx].firstTriIdx = i;
    bvhNode[rightChildIdx].triCount = node.triCount - leftCount;
    node.triCount = 0;
    UpdateNodeBounds( leftChildIdx );
    UpdateNodeBounds( rightChildIdx );
    // recurse
    Subdivide( leftChildIdx );
    Subdivide( rightChildIdx );
}

void build(bvh_t &bvh, mesh_t &mesh, attrib_t &attributes)
{
    node_t& root = bvh.nodes[0];
    root.child_left  = 0;
    root.prim_first  = 0;
    root.prim_count  = mesh.indices.size() / 3;

    update_bounds(bvh.nodes[0], mesh, attributes);
    subdivide(bvh, 0);
}
 */

        } /* namespace bvh */
    } /* namespace spatial */
} /* namespace xtcore */
