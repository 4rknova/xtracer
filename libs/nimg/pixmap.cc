#include "pixmap.h"

namespace nimg {

Pixmap::Pixmap()
	: m_width (0)
    , m_height(0)
{}

Pixmap::Pixmap(const Pixmap &img)
{
	if (&img == this) return;

	m_pixels = img.m_pixels;
	m_width  = img.width();
	m_height = img.height();
}

Pixmap &Pixmap::operator =(const Pixmap &img)
{
	if (&img == this)
		return *this;

	m_pixels = img.m_pixels;
	m_width  = img.width();
	m_height = img.height();

	return *this;
}

Pixmap::~Pixmap()
{}

size_t Pixmap::width() const
{
	return m_width;
}

size_t Pixmap::height() const
{
	return m_height;
}

const ColorRGBAf &Pixmap::pixel_ro(size_t x, size_t y) const
{
	size_t nx = x >= m_width  ? m_width  - 1 : x;
	size_t ny = y >= m_height ? m_height - 1 : y;

	return m_pixels[ny * m_width + nx];
}

ColorRGBAf &Pixmap::pixel(size_t x, size_t y)
{
	size_t nx = x >= m_width  ? m_width  - 1 : x;
	size_t ny = y >= m_height ? m_height - 1 : y;

	return m_pixels[ny * m_width + nx];
}

int Pixmap::init(size_t w, size_t h)
{
	if (m_pixels.init(w*h)) return 1;

	m_width  = w;
	m_height = h;

	return 0;
}

} /* namespace nimg */
