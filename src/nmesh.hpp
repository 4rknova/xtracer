#ifndef NMESH_MESH_HPP_INCLUDED
#define NMESH_MESH_HPP_INCLUDED

#include "vertex.h"
#include "buffer.hpp"

namespace NMesh {

class Mesh
{
	public:
		Mesh();
		Mesh(const Mesh &m);
		Mesh &operator =(const Mesh &m);
		virtual ~Mesh();

		// RETURN CODES:
		//	0. Everything went well.
		//  1. Failed to initialize vertices.
		//  2. Failed to initialize indices.
		//  3. Failed to initialize both.
		int init(const unsigned int vcount, const unsigned int icount);

		const Buffer<vertex_t> &vertices_ro() const;
		const Buffer<index_t> &indices_ro() const;

		Buffer<vertex_t> &vertices();
		Buffer<index_t> &indices();

	private:
		Buffer<vertex_t> m_vertices;
		Buffer<index_t> m_indices;
};

} /* namespace NMesh */

#endif /* NMESH_MESH_HPP_INCLUDED */
