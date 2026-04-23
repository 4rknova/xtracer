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
    m_source_path = file ? std::string(file) : std::string();
    Log::handle().post_message("Loading texture %s (%i)", file, res);
    return res;
}

int Texture2D::load_memory(const unsigned char *data, size_t size)
{
    const int res = nimg::io::load::image_memory(data, size, m_map);
    m_source_path.clear();
    Log::handle().post_message("Loading texture from memory (%i)", res);
    return res;
}

int Texture2D::load(const nimg::Pixmap &map)
{
	m_map = map;
    m_source_path.clear();
	return 0;
}

const std::string &Texture2D::source_path() const
{
    return m_source_path;
}

size_t Texture2D::width() const
{
    return m_map.width();
}

size_t Texture2D::height() const
{
    return m_map.height();
}

const nimg::ColorRGBAf &Texture2D::pixel_ro(size_t x, size_t y) const
{
    return m_map.pixel_ro(x, y);
}

nimg::ColorRGBf Texture2D::sample(const nmath::Vector3f &tc) const
{
    switch (m_filtering) {
        case FILTERING_NEAREST  : return nimg::sample::nearest  (m_map, tc.x, tc.y);
        case FILTERING_LINEAR   :
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
