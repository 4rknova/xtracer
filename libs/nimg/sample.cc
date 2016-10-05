#include <cstddef>
#include <cmath>
#include "sample.h"

namespace nimg {
    namespace sample {

ColorRGBAf linear(const Pixmap &map, double u, double v)
{
	u = u < 0 ? 0 : (u > 1 ? u - floor(u) : u);
	v = v < 0 ? 0 : (v > 1 ? v - floor(v) : v);

	size_t x = (size_t)(u * (float)map.width() );
	size_t y = (size_t)(v * (float)map.height());

	return map.pixel_ro(x, y);
}

ColorRGBAf bilinear(const Pixmap &map, double u, double v)
{
	u = u < 0 ? 0 : (u > 1 ? u - floor(u) : u);
	v = v < 0 ? 0 : (v > 1 ? v - floor(v) : v);

	u = u * map.width()  - 0.5;
	v = v * map.height() - 0.5;

	size_t x = floor(u);
	size_t y = floor(v);

	double u_ratio = u - x;
	double v_ratio = v - y;

	double u_opposite = 1 - u_ratio;
   	double v_opposite = 1 - v_ratio;

   	return (map.pixel_ro(x, y  ) * u_opposite  + map.pixel_ro(x+1, y  ) * u_ratio) * v_opposite +
    	   (map.pixel_ro(x, y+1) * u_opposite  + map.pixel_ro(x+1, y+1) * u_ratio) * v_ratio;
}

    } /* namespace sample */
} /* namespace nimg */
