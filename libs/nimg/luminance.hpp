#ifndef NIMG_LUMINANCE_HPP_INCLUDED
#define NIMG_LUMINANCE_HPP_INCLUDED

#include "color.hpp"

namespace NImg {
	namespace Operator {

float luminance(const ColorRGBf &color);
float luminance(const ColorRGBAf &color);

	} /* Operator */
} /* namespace NImg */

#endif /* NIMG_LUMINANCE_HPP_INCLUDED */
