#include "nimg/img.h"
#include "nimg/sample.h"
#include "texture.h"

namespace xtracer {
    namespace assets {

Texture2D::Texture2D()
    : m_filtering(FILTERING_NEAREST)
{}

void Texture2D::set_filtering(FILTERING filtering)
{
    m_filtering = filtering;
}

int Texture2D::load(const char *file)
{
	return nimg::io::load::image(file, m_map);
}

int Texture2D::load(const Pixmap &map)
{
	m_map = map;
	return 0;
}

ColorRGBAf Texture2D::sample(const float s, const float t) const
{
    switch (m_filtering) {
        case FILTERING_NEAREST  : return nimg::sample::nearest(m_map, s, t);
        case FILTERING_BILINEAR : return nimg::sample::bilinear(m_map, s, t);
    }

    return nimg::ColorRGBAf(0,0,0,1);
}

    } /* namespace assets */
} /* namespace xtracer */

