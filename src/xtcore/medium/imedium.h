#ifndef XTCORE_MEDIUM_IMEDIUM_H_INCLUDED
#define XTCORE_MEDIUM_IMEDIUM_H_INCLUDED

#include <nimg/color.h>
#include <nmath/precision.h>
#include <nmath/vector.h>

namespace xtcore {
namespace asset {
namespace medium {

class IMedium
{
public:
    IMedium();
    virtual ~IMedium();
    virtual IMedium *clone() const = 0;

    virtual nimg::ColorRGBf sigma_a() const = 0;
    virtual nimg::ColorRGBf sigma_s() const = 0;
    virtual nimg::ColorRGBf emission() const = 0;
    virtual nmath::scalar_t asymmetry() const = 0;

    virtual nimg::ColorRGBf sigma_a_at(const nmath::Vector3f &p) const;
    virtual nimg::ColorRGBf sigma_s_at(const nmath::Vector3f &p) const;
    virtual nimg::ColorRGBf emission_at(const nmath::Vector3f &p) const;
    virtual nimg::ColorRGBf sigma_t_at(const nmath::Vector3f &p) const;

    virtual nimg::ColorRGBf sigma_t() const;
    virtual nmath::scalar_t sigma_t_majorant() const;
    virtual nimg::ColorRGBf transmittance(const nmath::Vector3f &origin,
                                          const nmath::Vector3f &direction,
                                          nmath::scalar_t dist) const;
    virtual bool sample_distance(const nmath::Vector3f &origin,
                                 const nmath::Vector3f &direction,
                                 nmath::scalar_t segment_dist,
                                 nmath::scalar_t &sampled_dist) const;
    virtual nimg::ColorRGBf scattering_weight(const nmath::Vector3f &p) const;

    nimg::ColorRGBf transmittance(nmath::scalar_t dist) const;
    bool sample_distance(nmath::scalar_t segment_dist, nmath::scalar_t &sampled_dist) const;
    nimg::ColorRGBf scattering_weight() const;
};

} // namespace medium
} // namespace asset
} // namespace xtcore

#endif /* XTCORE_MEDIUM_IMEDIUM_H_INCLUDED */
