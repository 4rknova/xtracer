#ifndef XTRACER_OCTREE_HPP_INCLUDED
#define XTRACER_OCTREE_HPP_INCLUDED

#include <vector>
#include <list>
#include <nmath/ray.h>
#include <nmath/aabb.h>
#include <nmath/intinfo.h>

using NMath::Ray;
using NMath::BoundingBox3;
using NMath::IntInfo;

template <typename T>
class OctreeItem
{
	public:
		BoundingBox3 aabb;
		T data;
};

template <typename T>
class OctreeNode
{
	public:
		OctreeNode();
		unsigned int count_items();
	
		BoundingBox3 aabb;
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
		void add(const BoundingBox3 &aabb, const T &data);
		OctreeItem<T> *intersection(const Ray &ray, IntInfo *point) const;

		void max_items_per_node(unsigned int count);
		void max_depth(unsigned int depth);
		unsigned int max_items_per_node();
		unsigned int max_depth();

	private:
		OctreeItem<T> *r_intersection(OctreeNode<T> *node, const Ray &ray, IntInfo *point) const;
		void subdivide(OctreeNode<T> *node, unsigned int level);
		void release(OctreeNode<T> *node);	// Release a sub-tree.

        OctreeNode<T> *m_root;
		BoundingBox3 m_aabb;
		std::vector<OctreeItem<T> > m_items;

		unsigned int m_max_items_per_node;	// Maximum number of items per node.
		unsigned int m_max_depth;			// Maximum tree depth.
};

#include "octree.tml"

#endif /* XTRACER_BUFFER_HPP_INCLUDED */
