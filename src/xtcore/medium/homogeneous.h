#ifndef XTCORE_MEDIUM_HOMOGENEOUS_H_INCLUDED
#define XTCORE_MEDIUM_HOMOGENEOUS_H_INCLUDED

#include "imedium.h"

namespace xtcore {
namespace asset {
namespace medium {

class Homogeneous : public IMedium
{
public:
    Homogeneous();
    Homogeneous(
          const nimg::ColorRGBf &sigma_a
        , const nimg::ColorRGBf &sigma_s
        , const nimg::ColorRGBf &emission
        , nmath::scalar_t g
    );
    ~Homogeneous() override;
    IMedium *clone() const override;

    nimg::ColorRGBf sigma_a() const override;
    nimg::ColorRGBf sigma_s() const override;
    nimg::ColorRGBf emission() const override;
    nmath::scalar_t asymmetry() const override;

private:
    nimg::ColorRGBf m_sigma_a;
    nimg::ColorRGBf m_sigma_s;
    nimg::ColorRGBf m_emission;
    nmath::scalar_t m_g;
};

} // namespace medium
} // namespace asset
} // namespace xtcore

#endif /* XTCORE_MEDIUM_HOMOGENEOUS_H_INCLUDED */
