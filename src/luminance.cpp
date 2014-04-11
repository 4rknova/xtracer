/*

	This file is part of libnimg.

	luminance.cpp
	Luminance

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

#include "luminance.hpp"

namespace NImg {
	namespace Operator {

NMath::scalar_t luminance(const ColorRGBf &color)
{
	return 0.2126 * color.r() + 0.7152 * color.g() + 0.0722 * color.b();
}

NMath::scalar_t luminance(const ColorRGBAf &color)
{
	return 0.2126 * color.r() + 0.7152 * color.g() + 0.0722 * color.b();
}

	} /* namespace Operator */
} /* namespace NImg */
