#ifndef XTCORE_BRDF_PHONG_H_INCLUDED
#define XTCORE_BRDF_PHONG_H_INCLUDED

#include <nmath/precision.h>
#include <nmath/vector.h>
#include <nmath/intinfo.h>
#include <nimg/color.h>

inline nimg::ColorRGBf phong(const NMath::Vector3f &campos
                           , const NMath::Vector3f &lightpos
                           , const NMath::IntInfo *info
                           , const nimg::ColorRGBf &light
                           , const NMath::scalar_t ks
                           , const NMath::scalar_t kd
                           , const NMath::scalar_t specexp
                           , const nimg::ColorRGBf &diffuse
                           , const nimg::ColorRGBf &specular);

#include "brdf_phong.inl"

#endif /* XTCORE_BRDF_PHONG_H_INCLUDED */
