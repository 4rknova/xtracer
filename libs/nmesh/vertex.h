#ifndef NMESH_VERTEX_H_INCLUDED
#define NMESH_VERTEX_H_INCLUDED

#include <nmath/precision.h>

namespace NMesh {

#ifdef __cplusplus
	extern "C" {
#endif /* __cplusplus */

struct vertex_t
{
	vertex_t()
		: px(0), py(0), pz(0)
		, nx(0), ny(0), nz(0)
		, bx(0), by(0), bz(0)
 		, tx(0), ty(0), tz(0)
        , u(0), v(0)
		, r(0), g(0), b(0), a(1)
		, mat(0)
	{}

	NMath::scalar_t px, py, pz;	/* Position            */
	NMath::scalar_t nx, ny, nz;	/* Normal              */
	NMath::scalar_t bx, by, bz;	/* Binormal            */
	NMath::scalar_t tx, ty, tz;	/* Tangent             */
	NMath::scalar_t u, v;		/* Texture coordinates */
	NMath::scalar_t r, g, b, a;	/* Color               */
	int             mat;        /* Material ID         */
};

typedef struct vertex_t vertex_t;

typedef unsigned int index_t;

#ifdef __cplusplus
	} /* extern */
#endif /* __cplusplus */

} /* namespace NMesh */

#endif /* NMESH_VERTEX_H_INCLUDED */
