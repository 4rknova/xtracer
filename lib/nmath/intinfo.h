#ifndef XTCORE_INTINFO_H_INCLUDED
#define XTCORE_INTINFO_H_INCLUDED

#include <stdint.h>
#define HASH_UINT32 uint32_t
#define HASH_UINT64 uint64_t

#include "precision.h"
#include "vector.h"
#include "geometry.h"

namespace NMath {

class IntInfo
{
	public:
		IntInfo();

		Vector3f normal;
		Vector3f point;
		Vector3f texcoord;
		scalar_t t;

        HASH_UINT64 id;
};

} /* namespace NMath */

#endif /* XTCORE_INTINFO_H_INCLUDED */
