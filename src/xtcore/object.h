#ifndef XTCORE_OBJECT_H_INCLUDED
#define XTCORE_OBJECT_H_INCLUDED

#include "strpool.h"

namespace xtcore {
    namespace asset {

class IMaterial;
class ISurface;

class Object
{
	public:
    Object()
        : surface(0)
        , material(0)
        , ptr_material(0)
        , ptr_surface(0)
    {}

	HASH_UINT64 surface;
	HASH_UINT64 material;
    IMaterial *ptr_material;
    ISurface  *ptr_surface;
};

    } /* namespace asset */
} /* namespace xtcore */

#endif /* XTCORE_OBJECT_H_INCLUDED */
