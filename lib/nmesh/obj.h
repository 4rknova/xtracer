#ifndef NMESH_OBJ_HPP_INCLUDED
#define NMESH_OBJ_HPP_INCLUDED

#include "structs.h"

namespace nmesh {
		namespace io {
			namespace import {

int obj(const char *file, object_t &obj);

		} /* namespace import */
	} /* namespace io */
} /* namespace nmesh */

#endif /* NMESH_OBJ_HPP_INCLUDED */
