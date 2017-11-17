#ifndef XTCORE_SAMPLER_TEX_H_INCLUDED
#define XTCORE_SAMPLER_TEX_H_INCLUDED

#include <nmath/vector.h>
#include <nimg/color.h>
#include <nimg/pixmap.h>
#include "sampler.h"

namespace xtcore {
    namespace assets {

class Texture2D : public ISampler
{
	public:
	int load(const char *file);
	int load(const nimg::Pixmap &map);

    void set_filtering(FILTERING filtering);

	virtual nimg::ColorRGBf sample(const NMath::Vector3f &tc) const;

    void flip_horizontal();
    void flip_vertical();

    Texture2D();
    virtual ~Texture2D();

	private:
    FILTERING    m_filtering;
  	nimg::Pixmap m_map;
};

    } /* namespace assets */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_TEX_H_INCLUDED */
