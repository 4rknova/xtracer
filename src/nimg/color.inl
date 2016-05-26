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


inline ColorRGBf &ColorRGBf::operator *=(const float s)
{
	r(r() * s);
	g(g() * s);
	b(b() * s);

	return *this;
}

inline void ColorRGBf::r(const float r)
{
	m_r = (r < 0.f ? 0.f : r);
}

inline void ColorRGBf::g(const float g)
{
	m_g = (g < 0.f ? 0.f : g);
}

inline void ColorRGBf::b(const float b)
{
	m_b = (b < 0.f ? 0.f : b);
}

inline float ColorRGBf::r() const
{
	return m_r;
}

inline float ColorRGBf::g() const
{
	return m_g;
}

inline float ColorRGBf::b() const
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

inline ColorRGBf operator *(const ColorRGBf &lhs, const float s)
{
	return ColorRGBf(lhs) *= s;
}

inline ColorRGBf operator *(const float s, const ColorRGBf &lhs)
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

inline ColorRGBAf &ColorRGBAf::operator *=(const float s)
{
	r(r() * s);
	g(g() * s);
	b(b() * s);

	return *this;
}

inline void ColorRGBAf::r(const float r)
{
	m_r = (r < 0.f ? 0.f : r);
}

inline void ColorRGBAf::g(const float g)
{
	m_g = (g < 0.f ? 0.f : g);
}

inline void ColorRGBAf::b(const float b)
{
	m_b = (b < 0.f ? 0.f : b);
}

inline void ColorRGBAf::a(const float a)
{
	m_a = (a < 0.f ? 0.f : (a > 1.f ? 1.f : a));
}

inline float ColorRGBAf::r() const
{
	return m_r;
}

inline float ColorRGBAf::g() const
{
	return m_g;
}

inline float ColorRGBAf::b() const
{
	return m_b;
}

inline float ColorRGBAf::a() const
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

inline ColorRGBAf operator *(const ColorRGBAf &lhs, const float s)
{
	return ColorRGBAf(lhs) *= s;
}

inline ColorRGBAf operator *(const float s, const ColorRGBAf &lhs)
{
	return ColorRGBAf(lhs) *= s;
}

} /* namespace NImg */

#endif /* NIMG_COLOR_INL_INCLUDED */
