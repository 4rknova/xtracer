#include <nmath/vector.h>
#include <nmath/matrix.h>
#include "transform.h"

namespace NMesh {
	namespace Mutator {

void translate(object_t &object, NMath::scalar_t x, NMath::scalar_t y, NMath::scalar_t z)
{
	for (size_t i=0; i < object.attributes.v.size(); i+=3) {
        object.attributes.v[i  ] += x;
        object.attributes.v[i+1] += y;
        object.attributes.v[i+2] += z;
	}
}

void scale(object_t &object, NMath::scalar_t x, NMath::scalar_t y, NMath::scalar_t z)
{
	for (size_t i=0; i < object.attributes.v.size(); i+=3) {
        object.attributes.v[i  ] *= x;
        object.attributes.v[i+1] *= y;
        object.attributes.v[i+2] *= z;
	}
}

void rotate(object_t &object, NMath::scalar_t x, NMath::scalar_t y, NMath::scalar_t z)
{
    NMath::Matrix4x4f mat;
	mat.set_rotation(NMath::Vector3f(x,y,z));

	for (size_t i=0; i < object.attributes.v.size(); i+=3) {
        NMath::Vector3f v(object.attributes.v[i  ]
                        , object.attributes.v[i+1]
                        , object.attributes.v[i+2]);

		v.transform(mat);

        object.attributes.v[i  ] = v.x;
        object.attributes.v[i+1] = v.y;
        object.attributes.v[i+2] = v.z;
    }

    for (size_t i=0; i < object.attributes.n.size(); i+=3) {
        NMath::Vector3f n(object.attributes.n[i  ]
                        , object.attributes.n[i+1]
                        , object.attributes.n[i+2]);

   		n.transform(mat);

        object.attributes.n[i  ] = n.x;
        object.attributes.n[i+1] = n.y;
        object.attributes.n[i+2] = n.z;
	}
}

		} /* namespace Mutator */
} /* namespace NMesh */
