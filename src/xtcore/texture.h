#ifndef XTRACER_TEXTURE_H_INCLUDED
#define XTRACER_TEXTURE_H_INCLUDED

#include "nimg/color.h"
#include "nimg/pixmap.h"

using nimg::ColorRGBAf;
using nimg::Pixmap;

class Texture2D
{
	public:
		// RETURN CODES:
		//	0. Everything went well.
		//	1. Failed to load the texture data.
		unsigned int load(const char *file);
		unsigned int load(const Pixmap &map);

		const ColorRGBAf &sample(const float s, const float t) const;

	private:
		Pixmap m_map;
};

#endif /* XTRACER_TEXTURE_H_INCLUDED */
