#ifndef XTRACER_MAT_BLINNPHONG_H_INCLUDED
#define XTRACER_MAT_BLINNPHONG_H_INCLUDED

#include "material.h"

class MatBlinnPhong : public xtracer::assets::IMaterial
{
	public:
		void shade(ColorRGBf &res, Vector3f cam_position, const ILight *light, ColorRGBf &texcolor, const IntInfo &info);
};

#endif /* XTRACER_MAT_BLINNPHONG_H_INCLUDED */
