#include <nimg/img.h>
#include <nimg/sample.h>
#include "sampler_tex.h"
#include <cstdio>

namespace xtcore {
    namespace assets {

Texture2D::Texture2D()
    : m_filtering(FILTERING_NEAREST)
{}

Texture2D::~Texture2D()
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

void Texture2D::applu_multiplier(float multiplier)
{
    for (size_t x = 0; x < m_map.width(); ++x) {
        for (size_t y = 0; y < m_map.height(); ++y) {
            m_map.pixel(x,y) = multiplier * m_map.pixel_ro(x,y);
        }
    }
}

nimg::ColorRGBf Texture2D::sample(const NMath::Vector3f &tc) const
{
    switch (m_filtering) {
        case FILTERING_NEAREST  : return nimg::sample::nearest  (m_map, tc.x, tc.y);
        case FILTERING_BILINEAR : return nimg::sample::bilinear (m_map, tc.x, tc.y);
    }

    return nimg::ColorRGBf(0,0,0);
}

    } /* namespace assets */
} /* namespace xtcore */