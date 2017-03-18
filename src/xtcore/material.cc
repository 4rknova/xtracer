#include <nimg/luminance.h>
#include "log.h"
#include "material.h"

namespace xtracer {
    namespace assets {

IMaterial::IMaterial()
{}

void purge(std::map<std::string, NMath::scalar_t> &map)
{
	if(!map.empty()) map.clear();
}

void purge(std::map<std::string, ISampler*> &map)
{
	if(!map.empty()) {
		for (typename std::map<std::string, ISampler*>::iterator it = map.begin(); it != map.end(); ++it) {
			Log::handle().post_debug("Releasing %s..", (*it).first.c_str());
			delete (*it).second;
		}
		map.clear();
	}
}

int purge(std::map<std::string, NMath::scalar_t> &map, const char *name)
{
	typename std::map<std::string, NMath::scalar_t>::iterator it = map.find(name);
	if (it == map.end()) return 1;
	map.erase(it);
	return 0;
}

int purge(std::map<std::string, ISampler*> &map, const char *name)
{
	typename std::map<std::string, ISampler*>::iterator it = map.find(name);
	if (it == map.end()) return 1;
    delete (*it).second;
	map.erase(it);
	return 0;
}

IMaterial::~IMaterial()
{
    purge(m_scalars);
    purge(m_samplers);
}

bool IMaterial::is_emissive() const
{
    return nimg::eval::luminance(get_sample(MAT_SAMPLER_EMISSIVE, NMath::Vector3f(0,0,0))) > 0;
}

NMath::scalar_t IMaterial::get_scalar(const char *name) const
{
    const std::map<std::string, NMath::scalar_t>::const_iterator it = m_scalars.find(name);
    if (it == m_scalars.end()) return 0.f;
    return (*it).second;
}

nimg::ColorRGBf IMaterial::get_sample(const char *name, const NMath::Vector3f &tc) const
{
    const std::map<std::string, ISampler*>::const_iterator it = m_samplers.find(name);
    if (it == m_samplers.end()) return nimg::ColorRGBf(0,0,0);
    return (*it).second->sample(tc);
}

int IMaterial::purge_sampler(const char *name)
{
    return purge(m_samplers, name);
}

int IMaterial::purge_scalar(const char *name)
{
    return purge(m_scalars, name);
}

int IMaterial::add_scalar(const char *name, NMath::scalar_t scalar)
{
    int res = purge_scalar(name);
    m_scalars[name] = scalar;
    return res == 1 ? 0 : 1;
}

int IMaterial::add_sampler(const char *name, ISampler *sampler)
{
    int res = purge_sampler(name);
    m_samplers[name] = sampler;

    return res == 1 ? 0 : 1;
}

    } /* namespace assets */
} /* namespace xtracer */
