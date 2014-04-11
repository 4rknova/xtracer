/*

	This file is part of libnmesh.

	mesh.cpp
	Mesh

	Copyright (C) 2008, 2010 - 2012
	Papadopoulos Nikolaos

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 3 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General
	Public License along with this program; if not, write to the
	Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
	Boston, MA 02110-1301 USA

*/

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
