#include "geometry.h"

namespace NMath {

Geometry::Geometry(XTCORE_GEOMETRY_TYPE t)
	: type(t)
    , locality(XTCORE_GEOMETRY_STATIC)
	, uv_scale(Vector2f(1,1))
{}

Geometry::~Geometry()
{}

} /* namespace NMath */
