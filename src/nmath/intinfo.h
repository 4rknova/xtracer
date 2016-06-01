#ifndef XTCORE_INTINFO_H_INCLUDED
#define XTCORE_INTINFO_H_INCLUDED

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
		Vector2f texcoord;
		scalar_t t;
		const Geometry* geometry;
};

} /* namespace NMath */

#endif /* XTCORE_INTINFO_H_INCLUDED */
