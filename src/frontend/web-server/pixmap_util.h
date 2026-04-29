#ifndef XTRACER_FRONTEND_WEB_PIXMAP_UTIL_H_INCLUDED
#define XTRACER_FRONTEND_WEB_PIXMAP_UTIL_H_INCLUDED

#include <vector>
#include <nimg/pixmap.h>

namespace xtracer {
namespace frontend {
namespace web {

// Converts a Pixmap (linear float) to packed sRGB u8 RGBA, row-major, alpha=255.
// Tonemapping and post-filters must already be applied before calling.
void encode_rgba8_srgb(const nimg::Pixmap &pixmap, std::vector<unsigned char> &out);

} /* namespace web */
} /* namespace frontend */
} /* namespace xtracer */

#endif /* XTRACER_FRONTEND_WEB_PIXMAP_UTIL_H_INCLUDED */
