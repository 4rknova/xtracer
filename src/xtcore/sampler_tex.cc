#include <nimg/img.h>
#include <nimg/sample.h>
#include <nimg/transform.h>
#include "sampler_tex.h"
#include "log.h"
#include <cstdio>

namespace xtcore {
    namespace sampler {

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
    int res = nimg::io::load::image(file, m_map);
    Log::handle().post_debug("Loading texture: %s (%i)", file, res);
    return res;
}

int Texture2D::load(const nimg::Pixmap &map)
{
	m_map = map;
	return 0;
}

nimg::ColorRGBf Texture2D::sample(const NMath::Vector3f &tc) const
{
    switch (m_filtering) {
        case FILTERING_NEAREST  : return nimg::sample::nearest  (m_map, tc.x, tc.y);
        case FILTERING_BILINEAR : return nimg::sample::bilinear (m_map, tc.x, tc.y);
    }

    return nimg::ColorRGBf(0,0,0);
}

void Texture2D::flip_horizontal()
{
    nimg::transform::flip_horizontal(&m_map);
}

void Texture2D::flip_vertical()
{
    nimg::transform::flip_vertical(&m_map);
}

    } /* namespace sampler */
} /* namespace xtcore */
