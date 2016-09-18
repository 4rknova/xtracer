#ifndef NMESH_OBJ_HPP_INCLUDED
#define NMESH_OBJ_HPP_INCLUDED

#include "structs.h"

namespace NMesh {
		namespace IO {
			namespace Import {

int obj(const char *file, object_t &obj);

		} /* namespace Import */
	} /* namespace IO */
} /* namespace NMesh */

#endif /* NMESH_OBJ_HPP_INCLUDED */
