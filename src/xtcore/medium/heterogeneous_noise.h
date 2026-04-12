#ifndef XTCORE_MEDIUM_HETEROGENEOUS_NOISE_H_INCLUDED
#define XTCORE_MEDIUM_HETEROGENEOUS_NOISE_H_INCLUDED

#include "imedium.h"

namespace xtcore {
namespace asset {
namespace medium {

class HeterogeneousNoise : public IMedium
{
public:
    HeterogeneousNoise();
    HeterogeneousNoise(
          const nimg::ColorRGBf &sigma_a
        , const nimg::ColorRGBf &sigma_s
        , const nimg::ColorRGBf &emission
        , nmath::scalar_t g
        , nmath::scalar_t density_multiplier
        , nmath::scalar_t noise_scale
        , nmath::scalar_t noise_min
        , nmath::scalar_t noise_max
        , int octaves
        , nmath::scalar_t lacunarity
        , nmath::scalar_t gain
        , int seed
    );
    ~HeterogeneousNoise() override;

    IMedium *clone() const override;

    nimg::ColorRGBf sigma_a() const override;
    nimg::ColorRGBf sigma_s() const override;
    nimg::ColorRGBf emission() const override;
    nmath::scalar_t asymmetry() const override;

    nimg::ColorRGBf sigma_a_at(const nmath::Vector3f &p) const override;
    nimg::ColorRGBf sigma_s_at(const nmath::Vector3f &p) const override;
    nimg::ColorRGBf emission_at(const nmath::Vector3f &p) const override;
    nmath::scalar_t sigma_t_majorant() const override;

    nimg::ColorRGBf transmittance(const nmath::Vector3f &origin,
                                  const nmath::Vector3f &direction,
                                  nmath::scalar_t dist) const override;

    bool sample_distance(const nmath::Vector3f &origin,
                         const nmath::Vector3f &direction,
                         nmath::scalar_t segment_dist,
                         nmath::scalar_t &sampled_dist) const override;

    nimg::ColorRGBf scattering_weight(const nmath::Vector3f &p) const override;

private:
    nmath::scalar_t density_at(const nmath::Vector3f &p) const;

private:
    nimg::ColorRGBf m_sigma_a;
    nimg::ColorRGBf m_sigma_s;
    nimg::ColorRGBf m_emission;
    nmath::scalar_t m_g;

    nmath::scalar_t m_density_multiplier;
    nmath::scalar_t m_noise_scale;
    nmath::scalar_t m_noise_min;
    nmath::scalar_t m_noise_max;
    int m_octaves;
    nmath::scalar_t m_lacunarity;
    nmath::scalar_t m_gain;
    int m_seed;
};

} // namespace medium
} // namespace asset
} // namespace xtcore

#endif /* XTCORE_MEDIUM_HETEROGENEOUS_NOISE_H_INCLUDED */
