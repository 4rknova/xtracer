#ifndef NMESH_TRANSFORM_HPP_INCLUDED
#define NMESH_TRANSFORM_HPP_INCLUDED

#include <nmath/precision.h>
#include "nmesh.hpp"

namespace NMesh {
	namespace Mutator {

// RETURN CODES:
// 0. Everything went well.
int scale(NMesh::Mesh &mesh, NMath::scalar_t x, NMath::scalar_t y, NMath::scalar_t z);
int translate(NMesh::Mesh &mesh, NMath::scalar_t x, NMath::scalar_t y, NMath::scalar_t z);

	} /* namespace Mutator */
} /* namespace NMesh */

#endif /* NMESH_TRANSFORM_HPP_INCLUDED */
