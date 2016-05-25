#ifndef XTRACER_TEXTURE_HPP_INCLUDED
#define XTRACER_TEXTURE_HPP_INCLUDED

#include "color.hpp"
#include "pixmap.h"

using NImg::ColorRGBAf;
using NImg::Pixmap;

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

#endif /* XTRACER_TEXTURE_HPP_INCLUDED */
