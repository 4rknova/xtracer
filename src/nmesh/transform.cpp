#include <nmath/vector.h>
#include <nmath/matrix.h>
#include "buffer.hpp"
#include "transform.hpp"

namespace NMesh {
	namespace Mutator {

int translate(NMesh::Mesh &mesh, NMath::scalar_t x, NMath::scalar_t y, NMath::scalar_t z)
{
	Buffer<vertex_t> &p_meshv = mesh.vertices();

	for (unsigned int i = 0; i < p_meshv.count(); i++) {
		p_meshv[i].px += x;
		p_meshv[i].py += y;
		p_meshv[i].pz += z;
	}

	return 0;
}

int scale(NMesh::Mesh &mesh, NMath::scalar_t x, NMath::scalar_t y, NMath::scalar_t z)
{
	Buffer<vertex_t> &p_meshv = mesh.vertices();

	for (unsigned int i = 0; i < p_meshv.count(); i++) {
		p_meshv[i].px *= x;
		p_meshv[i].py *= y;
		p_meshv[i].pz *= z;
	}

	return 0;
}

int rotate(NMesh::Mesh &mesh, NMath::scalar_t x, NMath::scalar_t y, NMath::scalar_t z)
{
	Buffer<vertex_t> &p_meshv = mesh.vertices();
	NMath::Matrix4x4f mat;
	mat.set_rotation(NMath::Vector3f(x,y,z));

	for (unsigned int i = 0; i < p_meshv.count(); i++) {
		NMath::Vector3f v(p_meshv[i].px, p_meshv[i].py, p_meshv[i].pz);
		NMath::Vector3f n(p_meshv[i].nx, p_meshv[i].ny, p_meshv[i].nz);
		v.transform(mat);
		n.transform(mat);
		p_meshv[i].px = v.x;
		p_meshv[i].py = v.y;
		p_meshv[i].pz = v.z;
		p_meshv[i].nx = n.x;
		p_meshv[i].ny = n.y;
		p_meshv[i].nz = n.z;
	}

	return 0;
}

		} /* namespace Mutator */
} /* namespace NMesh */
