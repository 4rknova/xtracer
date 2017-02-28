#ifndef XTRACER_MATERIAL_H_INCLUDED
#define XTRACER_MATERIAL_H_INCLUDED

#include <map>
#include <nimg/color.h>
#include <nmath/intinfo.h>
#include "sampler.h"
#include "texture.h"
#include "camera.h"

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
        , const nimg::ColorRGBf &texcolor
        , const NMath::IntInfo &info) const = 0;

    NMath::scalar_t get_scalar (const char *name, NMath::Vector3f uvw);
    nimg::ColorRGBf get_color  (const char *name, NMath::Vector3f uvw);

    private:
    std::map<std::string, NMath::scalar_t>   m_scalars;
    std::map<std::string, ISampler*>         m_samplers;

public:
	nimg::ColorRGBf ambient;		// ambient intensity
	nimg::ColorRGBf diffuse;		// diffuse intensity
	nimg::ColorRGBf specular;		// specular intensity
	nimg::ColorRGBf emissive;		// emissive intensity

	NMath::scalar_t kspec;			// specular constant
	NMath::scalar_t kdiff;			// diffuse constant
	NMath::scalar_t ksexp;			// specular exponential
	NMath::scalar_t roughness;		// roughness (Ideally equal to exponent)

	NMath::scalar_t reflectance;	// reflectance
	NMath::scalar_t transparency;	// transparency
	NMath::scalar_t ior;			// index of refraction

};

    } /* namespace assets */
} /* namespace xtracer */

#endif /* XTRACER_MATERIAL_H_INCLUDED */
