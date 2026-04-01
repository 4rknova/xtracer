#ifndef XTCORE_MATH_SAMPLING_UTIL_H_INCLUDED
#define XTCORE_MATH_SAMPLING_UTIL_H_INCLUDED

#include <nmath/precision.h>
#include <nmath/prng.h>
#include <nmath/vector.h>
#include <nimg/color.h>

namespace xtcore {
namespace math {
namespace sampling {

nmath::Vector3f build_tangent(const nmath::Vector3f &n);
nmath::Vector3f sample_uniform_sphere(nmath::scalar_t &out_pdf);
nmath::scalar_t uniform_sphere_pdf();
nmath::Vector3f sample_cosine_hemisphere(const nmath::Vector3f &normal, nmath::scalar_t &out_pdf);
nmath::Vector3f sample_power_cosine_lobe(const nmath::Vector3f &axis,
                                         nmath::scalar_t exponent,
                                         nmath::scalar_t &out_pdf);
nmath::scalar_t power_cosine_lobe_pdf(const nmath::Vector3f &axis,
                                      const nmath::Vector3f &dir,
                                      nmath::scalar_t exponent);
nmath::Vector3f sample_ggx_half_vector(const nmath::Vector3f &normal,
                                       nmath::scalar_t roughness,
                                       nmath::scalar_t &out_pdf);
nmath::Vector3f sample_ggx_half_vector_anisotropic(const nmath::Vector3f &normal,
                                                   const nmath::Vector3f &tangent,
                                                   const nmath::Vector3f &bitangent,
                                                   nmath::scalar_t alpha_x,
                                                   nmath::scalar_t alpha_y,
                                                   nmath::scalar_t &out_pdf);
nmath::scalar_t ggx_ndf(const nmath::Vector3f &normal,
                        const nmath::Vector3f &half_vector,
                        nmath::scalar_t roughness);
nmath::scalar_t ggx_ndf_anisotropic(const nmath::Vector3f &normal,
                                    const nmath::Vector3f &tangent,
                                    const nmath::Vector3f &bitangent,
                                    const nmath::Vector3f &half_vector,
                                    nmath::scalar_t alpha_x,
                                    nmath::scalar_t alpha_y);
nmath::scalar_t smith_ggx_g1(const nmath::Vector3f &normal,
                             const nmath::Vector3f &dir,
                             nmath::scalar_t roughness);
nmath::scalar_t smith_ggx_g1_anisotropic(const nmath::Vector3f &normal,
                                         const nmath::Vector3f &tangent,
                                         const nmath::Vector3f &bitangent,
                                         const nmath::Vector3f &dir,
                                         nmath::scalar_t alpha_x,
                                         nmath::scalar_t alpha_y);
nmath::scalar_t smith_ggx_g(const nmath::Vector3f &normal,
                            const nmath::Vector3f &wo,
                            const nmath::Vector3f &wi,
                            nmath::scalar_t roughness);
nmath::scalar_t smith_ggx_g_anisotropic(const nmath::Vector3f &normal,
                                        const nmath::Vector3f &tangent,
                                        const nmath::Vector3f &bitangent,
                                        const nmath::Vector3f &wo,
                                        const nmath::Vector3f &wi,
                                        nmath::scalar_t alpha_x,
                                        nmath::scalar_t alpha_y);
nmath::scalar_t fresnel_dielectric(nmath::scalar_t cos_theta_i,
                                   nmath::scalar_t eta_i,
                                   nmath::scalar_t eta_t);
nimg::ColorRGBf fresnel_schlick(const nimg::ColorRGBf &f0, nmath::scalar_t cos_theta);

} // namespace sampling
} // namespace math
} // namespace xtcore

#endif /* XTCORE_MATH_SAMPLING_UTIL_H_INCLUDED */
