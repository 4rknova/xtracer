#ifndef XTRACER_TEXTURE_HPP_INCLUDED
#define XTRACER_TEXTURE_HPP_INCLUDED

#include <nimg/color.hpp>
#include <nimg/framebuffer.hpp>

using NImg::ColorRGBAf;
using NImg::Framebuffer;

class Texture2D
{
	public:
		// RETURN CODES:
		//	0. Everything went well.
		//	1. Failed to load the texture data.
		unsigned int load(const char *file);
		unsigned int load(const Framebuffer &fb);

		const ColorRGBAf &sample(const float s, const float t) const;
		
	private:
		Framebuffer m_fb;
};

#endif /* XTRACER_TEXTURE_HPP_INCLUDED */
