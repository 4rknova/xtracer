#ifndef XTCORE_SAMPLER_CUBEMAP_H_INCLUDED
#define XTCORE_SAMPLER_CUBEMAP_H_INCLUDED

#include <nmath/vector.h>
#include "sampler_tex.h"
#include "sampler.h"

namespace xtcore {
    namespace sampler {

enum CUBEMAP_FACE
{
      CUBEMAP_FACE_LEFT
    , CUBEMAP_FACE_RIGHT
    , CUBEMAP_FACE_TOP
    , CUBEMAP_FACE_BOTTOM
    , CUBEMAP_FACE_FRONT
    , CUBEMAP_FACE_BACK
};

class Cubemap : public ISampler
{
    public:
    int load(const char *file, CUBEMAP_FACE face);

    nimg::ColorRGBf sample(const nmath::Vector3f &tc) const;

    private:
    Texture2D m_textures[6]; // The 6 faces for the cubemap
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_CUBEMAP_H_INCLUDED */
