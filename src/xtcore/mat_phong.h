#ifndef XTRACER_MAT_PHONG_H_INCLUDED
#define XTRACER_MAT_PHONG_H_INCLUDED

#include "material.h"

class MatPhong : public xtracer::assets::IMaterial
{
	public:
		void shade(ColorRGBf &res, Vector3f cam_position, const ILight *light, ColorRGBf &texcolor, const IntInfo &info);
};

#endif /* XTRACER_MAT_PHONG_H_INCLUDED */
