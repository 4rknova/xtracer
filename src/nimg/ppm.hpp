#ifndef NIMG_PPM_HPP_INCLUDED
#define NIMG_PPM_HPP_INCLUDED


#include "pixmap.h"

namespace NImg {
	namespace IO {
		namespace Import {
//  RESTRICTIONS:
//  Only images with a pixel component maximum value of
//  255 are supported. This is the most common format.
//
// RETURN CODES:
// 0. Everything went well.
// 1. Filename is NULL.
// 2. File I/O error.
// 3. Invalid format.
// 4. Corrupted file.
// 5. Failed to initialize the Image.
int ppm_raw(const char *filename, Pixmap &map);

		} /* namespace Import */

		namespace Export {
//  RESTRICTIONS:
//  Only images with a pixel component maximum value of
//  255 are supported. This is the most common format.
//
// RETURN CODES:
// 0. Everything went well.
// 1. Filename is NULL.
// 2. File I/O error.
// 3. Nothing to export. Width or height is 0.
int ppm_raw(const char *filename, const Pixmap &map);

		} /* namespace Export */
	} /* namespace IO */
} /* namespace NImg */

#endif /* NIMG_PPM_HPP_INCLUDED */
