#ifndef NMESH_TRANSFORM_H_INCLUDED
#define NMESH_TRANSFORM_H_INCLUDED

#include <nmath/precision.h>
#include "structs.h"

namespace NMesh {
	namespace Mutator {

void scale    (object_t &object, NMath::scalar_t x, NMath::scalar_t y, NMath::scalar_t z);
void translate(object_t &object, NMath::scalar_t x, NMath::scalar_t y, NMath::scalar_t z);
void rotate   (object_t &object, NMath::scalar_t x, NMath::scalar_t y, NMath::scalar_t z);

	} /* namespace Mutator */
} /* namespace NMesh */

#endif /* NMESH_TRANSFORM_H_INCLUDED */
