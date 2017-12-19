#ifndef XTCORE_OBJECT_H_INCLUDED
#define XTCORE_OBJECT_H_INCLUDED

#include "strpool.h"

namespace xtcore {
    namespace assets {

class Object
{
	public:
	HASH_UINT64 geometry;
	HASH_UINT64 material;
};

    } /* namespace assets */
} /* namespace xtcore */

#endif /* XTCORE_OBJECT_H_INCLUDED */
