#include "color.h"

namespace nimg {

// ColorRGBf
ColorRGBf::ColorRGBf(const float r, const float g, const float b)
	: m_r(r < 0.f ? 0.f : r),
	  m_g(g < 0.f ? 0.f : g),
	  m_b(b < 0.f ? 0.f : b)
{}

ColorRGBf::ColorRGBf(const ColorRGBf &rhs)
	: m_r(rhs.r()), m_g(rhs.g()), m_b(rhs.b())
{}

ColorRGBf::ColorRGBf(const ColorRGBAf &rhs)
	: m_r(rhs.r()), m_g(rhs.g()), m_b(rhs.b())
{}

// ColorRGBAf
ColorRGBAf::ColorRGBAf(const float r, const float g, const float b, const float a)
	: m_r(r < 0.f ? 0.f : r),
	  m_g(g < 0.f ? 0.f : g),
	  m_b(b < 0.f ? 0.f : b),
	  m_a(a < 0.f ? 0.f : (a > 1.f ? 1.f : a))
{}

ColorRGBAf::ColorRGBAf(const ColorRGBf &rhs)
	: m_r(rhs.r()), m_g(rhs.g()), m_b(rhs.b()), m_a(1.0f)
{}

ColorRGBAf::ColorRGBAf(const ColorRGBAf &rhs)
	: m_r(rhs.r()), m_g(rhs.g()), m_b(rhs.b()), m_a(rhs.a())
{}

} /* namespace nimg */
