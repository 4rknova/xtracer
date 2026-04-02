#ifndef XTCORE_SAMPLER_TEX_H_INCLUDED
#define XTCORE_SAMPLER_TEX_H_INCLUDED

#include <nmath/vector.h>
#include <nimg/color.h>
#include <nimg/pixmap.h>
#include <string>
#include <xtcore/sampler.h>

using nmath::Vector3f;
using nimg::ColorRGBf;
using nimg::Pixmap;

namespace xtcore {
    namespace sampler {

class Texture2D : public ISampler
{
	public:
    int load(const char *file);
    int load_memory(const unsigned char *data, size_t size);
    int load(const Pixmap &map);
    const std::string &source_path() const;
    size_t width() const;
    size_t height() const;
    const nimg::ColorRGBAf &pixel_ro(size_t x, size_t y) const;

    void set_filtering(FILTERING filtering);

	virtual ColorRGBf sample(const Vector3f &tc) const;

    void flip_horizontal();
    void flip_vertical();

    Texture2D();
    virtual ~Texture2D();

	private:
    FILTERING m_filtering;
  	Pixmap    m_map;
    std::string m_source_path;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_TEX_H_INCLUDED */
