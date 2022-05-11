#ifndef XTCORE_OBJ_HPP_INCLUDED
#define XTCORE_OBJ_HPP_INCLUDED

#include <nmesh/structs.h>

namespace nmesh {
	namespace io {
			namespace import {

int obj(const char *file, nmesh::object_t &obj, const char *dir = 0);

		} /* namespace import */
	} /* namespace io */
} /* namespace nmesh */

#endif /* XTCORE_OBJ_HPP_INCLUDED */
