#include "nmesh.hpp"

namespace NMesh {

Mesh::Mesh()
{}

Mesh::Mesh(const Mesh &m)
{
	if (&m == this)
		return;

	m_vertices = m.vertices_ro();
	m_indices = m.indices_ro();
}

Mesh &Mesh::operator =(const Mesh &m)
{
	if (&m == this)
		return *this;

	m_vertices = m.vertices_ro();
	m_indices = m.indices_ro();

	return *this;
}

Mesh::~Mesh()
{}

int Mesh::init(const unsigned int vcount, const unsigned int icount)
{
	int status = 0;

	if (m_vertices.init(vcount)) {
		status = 1;
	}

	if (m_indices.init(icount)) {
		status = (status ? 3 : 2);
	}

	return status;
}

const Buffer<vertex_t> &Mesh::vertices_ro() const
{
	return m_vertices;
}

const Buffer<index_t> &Mesh::indices_ro() const
{
	return m_indices;
}

Buffer<vertex_t> &Mesh::vertices()
{
	return m_vertices;
}

Buffer<index_t> &Mesh::indices()
{
	return m_indices;
}

} /* namespace NMesh */
