#ifndef LIBNMATH_MESH_HPP_INCLUDED
#define LIBNMATH_MESH_HPP_INCLUDED

#include <nmath/ray.h>
#include <nmath/geometry.h>
#include <nmath/triangle.h>
#include <nmath/octree.hpp>

#include <nmesh/nmesh.hpp>


using NMath::Ray;
using NMath::Geometry;
using NMath::Triangle;

// Mesh
class Mesh: public NMesh::Mesh, public Geometry
{
    public:
        Mesh();
		~Mesh();

		bool intersection(const Ray &ray, IntInfo* i_info) const;
		void calc_aabb();
		void build_octree();

	private:
		Octree<Triangle> m_octree;
};

#endif /* LIBNMATH_MESH_HPP_INCLUDED */
