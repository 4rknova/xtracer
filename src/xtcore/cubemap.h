#ifndef XTRACER_CUBEMAP_H_INCLUDED
#define XTRACER_CUBEMAP_H_INCLUDED

#include <nmath/vector.h>
#include "texture.h"

namespace xtracer {
    namespace assets {
        namespace textures {

enum CUBEMAP_FACE
{
      CUBEMAP_FACE_LEFT
    , CUBEMAP_FACE_RIGHT
    , CUBEMAP_FACE_TOP
    , CUBEMAP_FACE_BOTTOM
    , CUBEMAP_FACE_FRONT
    , CUBEMAP_FACE_BACK
};

class Cubemap
{
    public:
    int load(const char *file, CUBEMAP_FACE face);

    nimg::ColorRGBAf sample(const NMath::Vector3f &direction) const;

    private:
    Texture2D m_textures[6]; // The 6 faces for the cubemap
};

        } /* namespace textures */
    } /* namespace assets */
} /* namespace xtracer */

#endif /* XTRACER_CUBEMAP_H_INCLUDED */
