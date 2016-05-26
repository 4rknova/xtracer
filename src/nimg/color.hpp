/*

	This file is part of libnimg.

	color.hpp
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

#ifndef NIMG_COLOR_HPP_INCLUDED
#define NIMG_COLOR_HPP_INCLUDED

namespace NImg {

// Forward declarations
class ColorRGBf;
class ColorRGBAf;

// COLOR RGB
class ColorRGBf
{
	public:
		explicit ColorRGBf(const float r = 0.0f, const float g = 0.0f, const float b = 0.0f);
		ColorRGBf(const ColorRGBf &rhs);
		ColorRGBf(const ColorRGBAf &rhs);

		// Operators
		inline ColorRGBf &operator +=(const ColorRGBf &rhs);
		inline ColorRGBf &operator -=(const ColorRGBf &rhs);
		inline ColorRGBf &operator *=(const ColorRGBf &rhs);
		inline ColorRGBf &operator *=(const float s);

		inline void r(const float r);
		inline void g(const float g);
		inline void b(const float b);

		inline float r() const;
		inline float g() const;
		inline float b() const;

	private:
		float m_r, m_g, m_b;

};

// Auxiliary operators
inline ColorRGBf operator +(const ColorRGBf &lhs, const ColorRGBf &rhs);
inline ColorRGBf operator -(const ColorRGBf &lhs, const ColorRGBf &rhs);
inline ColorRGBf operator *(const ColorRGBf &lhs, const ColorRGBf &rhs);
inline ColorRGBf operator *(const ColorRGBf &lhs, const float s);
inline ColorRGBf operator *(const float s, const ColorRGBf &lhs);

/*
    COLOR RGBA
*/
class ColorRGBAf
{
	public:
		explicit ColorRGBAf(const float r = 0.0f, const float g = 0.0f, const float b = 0.0f, const float a = 1.0f);
		ColorRGBAf(const ColorRGBf &rhs);
		ColorRGBAf(const ColorRGBAf &rhs);

		// Operators
		inline ColorRGBAf &operator +=(const ColorRGBAf &rhs);
		inline ColorRGBAf &operator -=(const ColorRGBAf &rhs);
		inline ColorRGBAf &operator *=(const ColorRGBAf &rhs);
		inline ColorRGBAf &operator *=(const float s);

		void r(const float r);
		void g(const float g);
		void b(const float b);
		void a(const float a);

		float r() const;
		float g() const;
		float b() const;
		float a() const;

	private:
		float m_r, m_g, m_b, m_a;

};

// Auxiliary operators
inline ColorRGBAf operator +(const ColorRGBAf &lhs, const ColorRGBAf &rhs);
inline ColorRGBAf operator -(const ColorRGBAf &lhs, const ColorRGBAf &rhs);
inline ColorRGBAf operator *(const ColorRGBAf &lhs, const ColorRGBAf &rhs);
inline ColorRGBAf operator *(const ColorRGBAf &lhs, const float s);
inline ColorRGBAf operator *(const float s, const ColorRGBAf &lhs);

} /* namespace NImg */

#include "color.inl"

#endif /* NIMG_COLOR_HPP_INCLUDED */
