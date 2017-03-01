#ifndef XTRACER_TEXTURE_H_INCLUDED
#define XTRACER_TEXTURE_H_INCLUDED

#include <nmath/vector.h>
#include <nimg/color.h>
#include <nimg/pixmap.h>
#include "sampler.h"

namespace xtracer {
    namespace assets {

class Texture2D : public ISampler
{
	public:
	int load(const char *file);
	int load(const nimg::Pixmap &map);

    void set_filtering(FILTERING filtering);

	nimg::ColorRGBf sample(const NMath::Vector3f &tc) const;

    Texture2D();

	private:
    FILTERING    m_filtering;
  	nimg::Pixmap m_map;
};

    } /* namespace assets */
} /* namespace xtracer */

#endif /* XTRACER_TEXTURE_H_INCLUDED */
