/*

	This file is part of libnimg.

	color.cpp
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

#include "color.hpp"

namespace NImg {

// ColorRGBf
ColorRGBf::ColorRGBf(const NMath::scalar_t r, const NMath::scalar_t g, const NMath::scalar_t b)
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
ColorRGBAf::ColorRGBAf(const NMath::scalar_t r, const NMath::scalar_t g, const NMath::scalar_t b, const NMath::scalar_t a)
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

} /* namespace NImg */
