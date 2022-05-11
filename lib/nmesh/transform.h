#ifndef NMESH_TRANSFORM_H_INCLUDED
#define NMESH_TRANSFORM_H_INCLUDED

#include <nmath/precision.h>
#include "structs.h"

namespace nmesh {
	namespace mutator {

void scale    (object_t &object, nmath::scalar_t x, nmath::scalar_t y, nmath::scalar_t z);
void translate(object_t &object, nmath::scalar_t x, nmath::scalar_t y, nmath::scalar_t z);
void rotate   (object_t &object, nmath::scalar_t x, nmath::scalar_t y, nmath::scalar_t z);

	} /* namespace mutator */
} /* namespace nmesh */

#endif /* NMESH_TRANSFORM_H_INCLUDED */
