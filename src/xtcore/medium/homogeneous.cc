#include <new>

#include "homogeneous.h"

namespace xtcore {
namespace asset {
namespace medium {

Homogeneous::Homogeneous()
    : m_sigma_a(0.0f, 0.0f, 0.0f)
    , m_sigma_s(0.0f, 0.0f, 0.0f)
    , m_emission(0.0f, 0.0f, 0.0f)
    , m_g(0.0f)
{}

Homogeneous::Homogeneous(
      const nimg::ColorRGBf &sigma_a
    , const nimg::ColorRGBf &sigma_s
    , const nimg::ColorRGBf &emission
    , nmath::scalar_t g
)
    : m_sigma_a(sigma_a)
    , m_sigma_s(sigma_s)
    , m_emission(emission)
    , m_g(g)
{}

Homogeneous::~Homogeneous()
{}

IMedium *Homogeneous::clone() const
{
    return new (std::nothrow) Homogeneous(m_sigma_a, m_sigma_s, m_emission, m_g);
}

nimg::ColorRGBf Homogeneous::sigma_a() const
{
    return m_sigma_a;
}

nimg::ColorRGBf Homogeneous::sigma_s() const
{
    return m_sigma_s;
}

nimg::ColorRGBf Homogeneous::emission() const
{
    return m_emission;
}

nmath::scalar_t Homogeneous::asymmetry() const
{
    return m_g;
}

} // namespace medium
} // namespace asset
} // namespace xtcore
