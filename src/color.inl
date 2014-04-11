/*

	This file is part of libnimg.

	color.inl
	Color

	Copyright (C) 2012
	Papadopoulos Nikolaos

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 3 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General
	Public License along with this program; if not, write to the
	Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
	Boston, MA 02110-1301 USA

*/

#ifndef NIMG_COLOR_INL_INCLUDED
#define NIMG_COLOR_INL_INCLUDED

#ifndef NIMG_COLOR_HPP_INCLUDED
    #error "color.hpp must be included before color.inl"
#endif /* NIMG_COLOR_HPP_INCLUDED */

namespace NImg {

// ColorRGBf
inline ColorRGBf &ColorRGBf::operator +=(const ColorRGBf& rhs)
{
	r(r() + rhs.r());
	g(g() + rhs.g());
	b(b() + rhs.b());

	return *this;
}

inline ColorRGBf &ColorRGBf::operator -=(const ColorRGBf& rhs)
{
	r(r() - rhs.r());
	g(g() - rhs.g());
	b(b() - rhs.b());

	return *this;
}

inline ColorRGBf &ColorRGBf::operator *=(const ColorRGBf &rhs)
{
	r(r() * rhs.r());
	g(g() * rhs.g());
	b(b() * rhs.b());

	return *this;
}


inline ColorRGBf &ColorRGBf::operator *=(const NMath::scalar_t s)
{
	r(r() * s);
	g(g() * s);
	b(b() * s);

	return *this;
}

inline void ColorRGBf::r(const NMath::scalar_t r)
{
	m_r = (r < 0.f ? 0.f : r);
}

inline void ColorRGBf::g(const NMath::scalar_t g)
{
	m_g = (g < 0.f ? 0.f : g);
}

inline void ColorRGBf::b(const NMath::scalar_t b)
{
	m_b = (b < 0.f ? 0.f : b);
}

inline NMath::scalar_t ColorRGBf::r() const
{
	return m_r;
}

inline NMath::scalar_t ColorRGBf::g() const
{
	return m_g;
}

inline NMath::scalar_t ColorRGBf::b() const
{
	return m_b;
}

inline ColorRGBf operator +(const ColorRGBf &lhs, const ColorRGBf &rhs)
{
	return ColorRGBf(lhs) += rhs;
}

inline ColorRGBf operator -(const ColorRGBf &lhs, const ColorRGBf &rhs)
{
	return ColorRGBf(lhs) -= rhs;
}

inline ColorRGBf operator *(const ColorRGBf &lhs, const ColorRGBf &rhs)
{
	return ColorRGBf(lhs) *= rhs;
}

inline ColorRGBf operator *(const ColorRGBf &lhs, const NMath::scalar_t s)
{
	return ColorRGBf(lhs) *= s;
}

inline ColorRGBf operator *(const NMath::scalar_t s, const ColorRGBf &lhs)
{
	return ColorRGBf(lhs) *= s;
}

// ColorRGBAf
inline ColorRGBAf &ColorRGBAf::operator +=(const ColorRGBAf &rhs)
{
	r(r() + rhs.r());
	g(g() + rhs.g());
	b(b() + rhs.b());

	return *this;
}

inline ColorRGBAf &ColorRGBAf::operator -=(const ColorRGBAf &rhs)
{
	r(r() - rhs.r());
	g(g() - rhs.g());
	b(b() - rhs.b());

	return *this;
}

inline ColorRGBAf &ColorRGBAf::operator *=(const ColorRGBAf &rhs)
{
	r(r() * rhs.r());
	g(g() * rhs.g());
	b(b() * rhs.b());

	return *this;
}

inline ColorRGBAf &ColorRGBAf::operator *=(const NMath::scalar_t s)
{
	r(r() * s);
	g(g() * s);
	b(b() * s);

	return *this;
}

inline void ColorRGBAf::r(const NMath::scalar_t r)
{
	m_r = (r < 0.f ? 0.f : r);
}

inline void ColorRGBAf::g(const NMath::scalar_t g)
{
	m_g = (g < 0.f ? 0.f : g);
}

inline void ColorRGBAf::b(const NMath::scalar_t b)
{
	m_b = (b < 0.f ? 0.f : b);
}

inline void ColorRGBAf::a(const NMath::scalar_t a)
{
	m_a = (a < 0.f ? 0.f : (a > 1.f ? 1.f : a));
}

inline NMath::scalar_t ColorRGBAf::r() const
{
	return m_r;
}

inline NMath::scalar_t ColorRGBAf::g() const
{
	return m_g;
}

inline NMath::scalar_t ColorRGBAf::b() const
{
	return m_b;
}

inline NMath::scalar_t ColorRGBAf::a() const
{
	return m_a;
}

inline ColorRGBAf operator +(const ColorRGBAf &lhs, const ColorRGBAf &rhs)
{
	return ColorRGBAf(lhs) += rhs;
}

inline ColorRGBAf operator -(const ColorRGBAf &lhs, const ColorRGBAf &rhs)
{
	return ColorRGBAf(lhs) -= rhs;
}

inline ColorRGBAf operator *(const ColorRGBAf &lhs, const ColorRGBAf &rhs)
{
	return ColorRGBAf(lhs) *= rhs;
}

inline ColorRGBAf operator *(const ColorRGBAf &lhs, const NMath::scalar_t s)
{
	return ColorRGBAf(lhs) *= s;
}

inline ColorRGBAf operator *(const NMath::scalar_t s, const ColorRGBAf &lhs)
{
	return ColorRGBAf(lhs) *= s;
}

} /* namespace NImg */

#endif /* NIMG_COLOR_INL_INCLUDED */
