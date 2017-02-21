#ifndef XTRACER_TEXTURE_H_INCLUDED
#define XTRACER_TEXTURE_H_INCLUDED

#include "nimg/color.h"
#include "nimg/pixmap.h"

using nimg::ColorRGBAf;
using nimg::Pixmap;

namespace xtracer {
    namespace assets {

enum FILTERING
{
      FILTERING_NEAREST
    , FILTERING_BILINEAR
};

class Texture2D
{
	public:
	int load(const char *file);
	int load(const Pixmap &map);

    void set_filtering(FILTERING filtering);

	ColorRGBAf sample(const float s, const float t) const;

    Texture2D();

	private:
    FILTERING m_filtering;
  	Pixmap    m_map;
};

    } /* namespace assets */
} /* namespace xtracer */

#endif /* XTRACER_TEXTURE_H_INCLUDED */
