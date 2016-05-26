#include "nimg/ppm.hpp"
#include "texture.hpp"

unsigned int Texture2D::load(const char *file)
{
	if (NImg::IO::Import::ppm_raw(file, m_map)) {
		return 1;
	}

	return 0;
}

unsigned int Texture2D::load(const Pixmap &map)
{
	m_map = map;

	return 0;
}

const ColorRGBAf &Texture2D::sample(const float s, const float t) const
{
	// Restrict s,t to the [0, 1] range.
	float rs = s < 0.f ? 0.0f : (s > 1.0f ? s - ((float)((int)(s))) : s);
	float rt = s < 0.f ? 0.0f : (t > 1.0f ? t - ((float)((int)(t))) : t);

	int x = (unsigned int)(rs * (float)m_map.width());
	int y = (unsigned int)(rt * (float)m_map.height());

	return m_map.pixel_ro(x, y);
}
