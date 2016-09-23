#ifndef NIMG_PIXMAP_H_INCLUDED
#define NIMG_PIXMAP_H_INCLUDED

#include "buffer.h"
#include "color.h"

namespace nimg {

// For visual studio:
// Disable "<type> needs to have dll-interface to be used by clients"
// This warning refers to STL member variables which are private and
// therefore can be safely ignored.
#pragma warning(push)
#pragma warning(disable : 4251) // unreferenced formal parameter

class Pixmap
{
	public:
		Pixmap();
		Pixmap(const Pixmap &img);
		Pixmap &operator =(const Pixmap &img);
		~Pixmap();

		// NOTES:
		// If the user sets one or both of the dimensions as 0
		// it is equivalent to resetting the buffer as total size is 0.
		// RETURN CODES:
		//	0. Everything went well.
		//  1. Failed to initialize pixels.
		int init(const unsigned int w, const unsigned int h);

		unsigned int width() const;
		unsigned int height() const;

		const ColorRGBAf &pixel_ro(const unsigned int x, const unsigned int y) const;
		ColorRGBAf &pixel(const unsigned int x, const unsigned int y);

	private:
		unsigned int m_width, m_height;
		mutable util::Buffer<ColorRGBAf> m_pixels;
};

#pragma warning (pop)

} /* namespace nimg */

#endif /* NIMG_TEXTURE_H_INCLUDED */
