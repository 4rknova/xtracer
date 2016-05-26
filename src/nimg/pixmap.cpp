#include "pixmap.h"

namespace NImg {

Pixmap::Pixmap()
	: m_width(0), m_height(0)
{}

Pixmap::Pixmap(const Pixmap &img)
{
	if (&img == this)
		return;

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

unsigned int Pixmap::width() const
{
	return m_width;
}

unsigned int Pixmap::height() const
{
	return m_height;
}

const ColorRGBAf &Pixmap::pixel_ro(const unsigned int x, const unsigned int y) const
{
	int nx = x >= m_width ? m_width - 1 : x;
	int ny = y >= m_height ? m_height - 1 : y;

	return m_pixels[ny * m_width + nx];
}

ColorRGBAf &Pixmap::pixel(const unsigned int x, const unsigned int y)
{
	int nx = x >= m_width ? m_width - 1 : x;
	int ny = y >= m_height ? m_height - 1 : y;

	return m_pixels[ny * m_width + nx];
}

int Pixmap::init(const unsigned int w, const unsigned int h)
{
	if(m_pixels.init(w*h))
		return 1;

	m_width = w;
	m_height = h;

	return 0;
}

} /* namespace NImg */
