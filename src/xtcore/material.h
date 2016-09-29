#ifndef XTRACER_MATERIAL_H_INCLUDED
#define XTRACER_MATERIAL_H_INCLUDED

#include <string>
#include <vector>
#include <map>
#include <nimg/color.h>
#include <nmath/intinfo.h>

#include "camera.h"
#include "light.h"

using NMath::IntInfo;
using NMath::Vector3f;
using nimg::ColorRGBf;

namespace xtracer {
    namespace assets {

class IMaterial
{
	public:
		virtual ~IMaterial();
		virtual void shade(ColorRGBf &res, Vector3f cam_position, const ILight *light, ColorRGBf &texcolor, const IntInfo &info) = 0;

        std::map<std::string, NMath::Vector3f> vectors;
        std::map<std::string, NMath::scalar_t> scalars;
        std::map<std::string, nimg::ColorRGBf> colors;
        std::map<std::string, std::string>     textures;
};

    } /* namespace assets */
} /* namespace xtracer */

#endif /* XTRACER_MATERIAL_H_INCLUDED */
