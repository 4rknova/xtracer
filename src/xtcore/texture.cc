#include <nimg/img.h>
#include <nimg/sample.h>
#include "texture.h"
#include <cstdio>

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

int Texture2D::load(const nimg::Pixmap &map)
{
	m_map = map;
	return 0;
}

nimg::ColorRGBAf Texture2D::sample(const NMath::Vector3f &tc) const
{
    switch (m_filtering) {
        case FILTERING_NEAREST  : return nimg::sample::nearest  (m_map, tc.x, tc.y);
        case FILTERING_BILINEAR : return nimg::sample::bilinear (m_map, tc.x, tc.y);
    }

    return nimg::ColorRGBAf(0,0,0,1);
}

    } /* namespace assets */
} /* namespace xtracer */
