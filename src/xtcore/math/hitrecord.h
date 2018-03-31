#ifndef XTCORE_HITRECORD_H_INCLUDED
#define XTCORE_HITRECORD_H_INCLUDED

#include <stdint.h>
#include <nmath/precision.h>
#include <nmath/vector.h>

#define HASH_UINT32 uint32_t
#define HASH_UINT64 uint64_t

using NMath::scalar_t;
using NMath::Vector3f;

namespace xtcore {

class HitRecord
{
	public:
		HitRecord();

		Vector3f normal;
		Vector3f point;
		Vector3f texcoord;
		scalar_t t;

        HASH_UINT64 id;
};

} /* namespace xtcore */

#endif /* XTCORE_HITRECORD_H_INCLUDED */
