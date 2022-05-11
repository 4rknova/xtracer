#include "luminance.h"

namespace nimg {
	namespace eval {

float luminance(const ColorRGBf &color)
{
	return 0.2126 * color.r() + 0.7152 * color.g() + 0.0722 * color.b();
}

float luminance(const ColorRGBAf &color)
{
	return 0.2126 * color.r() + 0.7152 * color.g() + 0.0722 * color.b();
}

	} /* namespace eval */
} /* namespace nimg */
