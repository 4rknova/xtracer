#ifndef XTRACER_MATERIAL_H_INCLUDED
#define XTRACER_MATERIAL_H_INCLUDED

#include <map>
#include <nimg/color.h>
#include <nmath/intinfo.h>
#include "sampler.h"
#include "texture.h"
#include "camera.h"
#include "matdefs.h"

namespace xtracer {
    namespace assets {

class IMaterial
{
	public:
	IMaterial();
	virtual ~IMaterial();

    bool is_emissive() const;

	virtual nimg::ColorRGBf shade(
          const NMath::Vector3f &cam_position
        , const NMath::Vector3f &light_pos
        , const nimg::ColorRGBf &light_intensity
        , const NMath::IntInfo  &info) const = 0;

    NMath::scalar_t get_scalar(const char *name) const;
    nimg::ColorRGBf get_sample(const char *name, const NMath::Vector3f &tc) const;

    int add_sampler   (const char *name, ISampler *sampler);
    int add_scalar    (const char *name, NMath::scalar_t scalar);
    int purge_sampler (const char *name);
    int purge_scalar  (const char *name);

    private:
    std::map<std::string, NMath::scalar_t> m_scalars;
    std::map<std::string, ISampler*>       m_samplers;

public:
	NMath::scalar_t transparency;	// transparency
	NMath::scalar_t ior;			// index of refraction

};

    } /* namespace assets */
} /* namespace xtracer */

#endif /* XTRACER_MATERIAL_H_INCLUDED */
