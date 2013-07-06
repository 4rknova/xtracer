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

#include "precision.h"

namespace NImg {

// Forward declarations
class ColorRGBf;
class ColorRGBAf;

// COLOR RGB
class ColorRGBf
{
	public:
		explicit ColorRGBf(const NMath::scalar_t r = 0.0f, const NMath::scalar_t g = 0.0f, const NMath::scalar_t b = 0.0f);
		ColorRGBf(const ColorRGBf &rhs);
		ColorRGBf(const ColorRGBAf &rhs);

		// Operators
		inline ColorRGBf &operator +=(const ColorRGBf &rhs);
		inline ColorRGBf &operator -=(const ColorRGBf &rhs);
		inline ColorRGBf &operator *=(const ColorRGBf &rhs);
		inline ColorRGBf &operator *=(const NMath::scalar_t s);

		inline void r(const NMath::scalar_t r);
		inline void g(const NMath::scalar_t g);
		inline void b(const NMath::scalar_t b);

		inline NMath::scalar_t r() const;
		inline NMath::scalar_t g() const;
		inline NMath::scalar_t b() const;

	private:
		NMath::scalar_t m_r, m_g, m_b;

};

// Auxiliary operators
inline ColorRGBf operator +(const ColorRGBf &lhs, const ColorRGBf &rhs);
inline ColorRGBf operator -(const ColorRGBf &lhs, const ColorRGBf &rhs);
inline ColorRGBf operator *(const ColorRGBf &lhs, const ColorRGBf &rhs);
inline ColorRGBf operator *(const ColorRGBf &lhs, const NMath::scalar_t s);
inline ColorRGBf operator *(const NMath::scalar_t s, const ColorRGBf &lhs);

/*
    COLOR RGBA
*/
class ColorRGBAf
{
	public:
		explicit ColorRGBAf(const NMath::scalar_t r = 0.0f, const NMath::scalar_t g = 0.0f, const NMath::scalar_t b = 0.0f, const NMath::scalar_t a = 1.0f);
		ColorRGBAf(const ColorRGBf &rhs);
		ColorRGBAf(const ColorRGBAf &rhs);

		// Operators
		inline ColorRGBAf &operator +=(const ColorRGBAf &rhs);
		inline ColorRGBAf &operator -=(const ColorRGBAf &rhs);
		inline ColorRGBAf &operator *=(const ColorRGBAf &rhs);
		inline ColorRGBAf &operator *=(const NMath::scalar_t s);

		void r(const NMath::scalar_t r);
		void g(const NMath::scalar_t g);
		void b(const NMath::scalar_t b);
		void a(const NMath::scalar_t a);

		NMath::scalar_t r() const;
		NMath::scalar_t g() const;
		NMath::scalar_t b() const;
		NMath::scalar_t a() const;

	private:
		NMath::scalar_t m_r, m_g, m_b, m_a;

};

// Auxiliary operators
inline ColorRGBAf operator +(const ColorRGBAf &lhs, const ColorRGBAf &rhs);
inline ColorRGBAf operator -(const ColorRGBAf &lhs, const ColorRGBAf &rhs);
inline ColorRGBAf operator *(const ColorRGBAf &lhs, const ColorRGBAf &rhs);
inline ColorRGBAf operator *(const ColorRGBAf &lhs, const NMath::scalar_t s);
inline ColorRGBAf operator *(const NMath::scalar_t s, const ColorRGBAf &lhs);

} /* namespace NImg */

#include "color.inl"

#endif /* NIMG_COLOR_HPP_INCLUDED */
