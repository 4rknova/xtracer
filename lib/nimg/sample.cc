#include <cstddef>
#include <cmath>
#include "sample.h"

namespace nimg {
    namespace sample {

ColorRGBAf nearest(const Pixmap &map, double u, double v)
{
	u = u < 0 ? 0 : (u > 1 ? u - floor(u) : u);
	v = v < 0 ? 0 : (v > 1 ? v - floor(v) : v);

	size_t x = (size_t)(u * (float)map.width());
	size_t y = (size_t)(v * (float)map.height());

	return map.pixel_ro(x, y);
}

ColorRGBAf bilinear(const Pixmap &map, double u, double v)
{
	size_t w = map.width();
	size_t h = map.height();

	float fu = u * w;
	float fv = v * h;

    // find which pixels participate
	int u1 = ((int)fu) % w;
	int v1 = ((int)fv) % h;
	int u2 = (u1 + 1)  % w;
	int v2 = (v1 + 1)  % h;

	// calculate fractional parts of u and v
	float fracu = fu - floorf(fu);
	float fracv = fv - floorf(fv);

	// calculate weight factors
	float w1 = (1 - fracu) * (1 - fracv);
	float w2 = fracu * (1 - fracv);
	float w3 = (1 - fracu) * fracv;
	float w4 = fracu *  fracv;

	// fetch four texels
	ColorRGBAf c1 = map.pixel_ro(u1, v1);
	ColorRGBAf c2 = map.pixel_ro(u2, v1);
	ColorRGBAf c3 = map.pixel_ro(u1, v2);
	ColorRGBAf c4 = map.pixel_ro(u2, v2);

	// scale and sum the four colors
	return c1 * w1 + c2 * w2 + c3 * w3 + c4 * w4;
}

    } /* namespace sample */
} /* namespace nimg */
