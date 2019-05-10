#ifndef XTCORE_OCTREE_H_INCLUDED
#define XTCORE_OCTREE_H_INCLUDED

#include <vector>
#include <list>
#include "math/aabb.h"
#include "math/ray.h"
#include "math/hitrecord.h"

namespace xtcore {

template <typename T>
class OctreeItem
{
	public:
		AABB3 aabb;
		T data;
};

template <typename T>
class OctreeNode
{
	public:
		OctreeNode();
		size_t count_items();

		AABB3 aabb;
		std::list<OctreeItem<T>*> items;
		OctreeNode<T> *child[8];
};

template <typename T>
class Octree
{
	public:
		Octree();
		~Octree();

		void clear();
		void build();
		void add(const AABB3 &aabb, const T &data);
		OctreeItem<T> *intersection(const Ray &ray, hit_record_t *point) const;

		void max_items_per_node(size_t count);
		void max_depth(size_t depth);
		size_t max_items_per_node();
		size_t max_depth();

        AABB3 bbox();

	private:
		OctreeItem<T> *r_intersection(OctreeNode<T> *node, const Ray &ray, hit_record_t *point) const;
		void subdivide(OctreeNode<T> *node, size_t level);
		void release(OctreeNode<T> *node);	// Release a sub-tree.

        OctreeNode<T> *m_root;
		AABB3 m_aabb;
		std::vector<OctreeItem<T> > m_items;

		size_t m_max_items_per_node;// Maximum number of items per node.
		size_t m_max_depth;			// Maximum tree depth.
};

} /* namespace xtcore */

#include "octree.tml"

#endif /* XTCORE_BUFFER_H_INCLUDED */
