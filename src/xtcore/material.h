#ifndef XTCORE_MATERIAL_H_INCLUDED
#define XTCORE_MATERIAL_H_INCLUDED

#include <map>
#include <nimg/color.h>
#include "math/hitrecord.h"
#include "math/ray.h"
#include "sampler.h"
#include "camera.h"
#include "matdefs.h"
#include "emitter.h"

using nmath::Vector3f;
using nimg::ColorRGBf;
using xtcore::sampler::ISampler;

namespace xtcore {
    namespace asset {

class IMaterial
{
	public:
	IMaterial();
	virtual ~IMaterial();

    bool is_emissive() const;

	virtual bool shade(
                ColorRGBf    &intensity
        , const ICamera      *cam
        , const emitter_t    *emitter
        , const hit_record_t &hit_record) const = 0;

    virtual bool sample_path(
                hit_result_t &hit_result
        , const hit_record_t &hit_record
    ) const = 0;

    float     get_scalar(const char *name) const;
    ColorRGBf get_sample(const char *name, const Vector3f &tc) const;
       float& get_scalar_by_index  (size_t idx, std::string *name=0);
    ISampler* get_sampler_by_index (size_t idx, std::string *name=0);

    size_t get_scalar_count()  const;
    size_t get_sampler_count() const;

    int add_sampler   (const char *name, ISampler *sampler);
    int add_scalar    (const char *name, float scalar);
    int purge_sampler (const char *name);
    int purge_scalar  (const char *name);

    private:
    std::map<std::string, float>     m_scalars;
    std::map<std::string, ISampler*> m_samplers;

};

    } /* namespace asset */
} /* namespace xtcore */

#include "material/lambert.h"
#include "material/phong.h"
#include "material/blinnphong.h"
#include "material/emissive.h"
#include "material/dielectric.h"

#endif /* XTCORE_MATERIAL_H_INCLUDED */
