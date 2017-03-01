#include <nimg/luminance.h>
#include "log.h"
#include "material.h"

namespace xtracer {
    namespace assets {

IMaterial::IMaterial()
	: ambient(nimg::ColorRGBf(1, 1, 1))
	, diffuse(nimg::ColorRGBf(1, 1, 1))
	, specular(nimg::ColorRGBf(1, 1, 1))
	, emissive(nimg::ColorRGBf(0, 0, 0))
	, kspec(0.0)
	, kdiff(1.0)
	, ksexp(60)
	, roughness(0)
	, reflectance(0.0)
	, transparency(0.0)
	, ior(1.5)
{}

template<typename T>
void purge(std::map<std::string, T> &map)
{
	if(!map.empty()) map.clear();
}

template<typename T>
void purge(std::map<std::string, T*> &map)
{
	if(!map.empty()) {
		for (typename std::map<std::string, T*>::iterator it = map.begin(); it != map.end(); ++it) {
			Log::handle().post_debug("Releasing %s..", (*it).first.c_str());
			delete (*it).second;
		}
		map.clear();
	}
}

template<typename T>
int purge(std::map<std::string, T> &map, const char *name)
{
	typename std::map<std::string, T*>::iterator it = map.find(name);
	if (it == map.end()) return 1;
	map.erase(it);
	return 0;
}

template<typename T>
int purge(std::map<std::string, T*> &map, const char *name)
{
	typename std::map<std::string, T*>::iterator it = map.find(name);
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
    return nimg::eval::luminance(emissive) > 0;
}

NMath::scalar_t IMaterial::get_scalar(const char *name)
{
    std::map<std::string, NMath::scalar_t>::iterator it = m_scalars.find(name);
    return (it == m_scalars.end() ? 0.f : m_scalars[name]);
}

nimg::ColorRGBAf IMaterial::get_sampler(const char *name)
{
    std::map<std::string, ISampler*>::iterator it = m_samplers.find(name);
    return (it == m_samplers.end() ? nimg::ColorRGBAf(0,0,0,1) : m_samplers[name]);
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
