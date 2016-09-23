#ifndef NIMG_LUMINANCE_H_INCLUDED
#define NIMG_LUMINANCE_H_INCLUDED

#include "color.h"

namespace nimg {
	namespace eval {

float luminance(const ColorRGBf &color);
float luminance(const ColorRGBAf &color);

	} /* namespace eval */
} /* namespace nimg */

#endif /* NIMG_LUMINANCE_H_INCLUDED */
