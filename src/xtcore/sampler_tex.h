#ifndef XTCORE_SAMPLER_TEX_H_INCLUDED
#define XTCORE_SAMPLER_TEX_H_INCLUDED

#include <nmath/vector.h>
#include <nimg/color.h>
#include <nimg/pixmap.h>
#include "sampler.h"

using NMath::Vector3f;
using nimg::ColorRGBf;
using nimg::Pixmap;

namespace xtcore {
    namespace sampler {

class Texture2D : public ISampler
{
	public:
	int load(const char *file);
	int load(const Pixmap &map);

    void set_filtering(FILTERING filtering);

	virtual ColorRGBf sample(const Vector3f &tc) const;

    void flip_horizontal();
    void flip_vertical();

    Texture2D();
    virtual ~Texture2D();

	private:
    FILTERING m_filtering;
  	Pixmap    m_map;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_TEX_H_INCLUDED */
