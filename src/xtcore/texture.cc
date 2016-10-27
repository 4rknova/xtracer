#include "nimg/img.h"
#include "nimg/sample.h"
#include "texture.h"

unsigned int Texture2D::load(const char *file)
{
	return nimg::io::load::image(file, m_map);
}

unsigned int Texture2D::load(const Pixmap &map)
{
	m_map = map;
	return 0;
}

const ColorRGBAf Texture2D::sample(const float s, const float t) const
{
    return nimg::sample::linear(m_map, s, t);
}
