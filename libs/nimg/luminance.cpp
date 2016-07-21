#include "luminance.hpp"

namespace NImg {
	namespace Operator {

float luminance(const ColorRGBf &color)
{
	return 0.2126 * color.r() + 0.7152 * color.g() + 0.0722 * color.b();
}

float luminance(const ColorRGBAf &color)
{
	return 0.2126 * color.r() + 0.7152 * color.g() + 0.0722 * color.b();
}

	} /* namespace Operator */
} /* namespace NImg */
