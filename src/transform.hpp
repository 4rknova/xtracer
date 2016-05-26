#ifndef NMESH_TRANSFORM_HPP_INCLUDED
#define NMESH_TRANSFORM_HPP_INCLUDED

#include <nmath/precision.h>
#include "mesh.hpp"

namespace NMesh {
	namespace Mutator {

// RETURN CODES:
// 0. Everything went well.
int scale(Mesh &mesh, NMath::scalar_t x, NMath::scalar_t y, NMath::scalar_t z);
int translate(Mesh &mesh, NMath::scalar_t x, NMath::scalar_t y, NMath::scalar_t z);

	} /* namespace Mutator */
} /* namespace NMesh */

#endif /* NMESH_TRANSFORM_HPP_INCLUDED */
