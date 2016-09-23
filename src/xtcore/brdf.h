#ifndef XTCORE_BRDF_H_INCLUDED
#define XTCORE_BRDF_H_INCLUDED

#include <nimg/color.h>
#include "brdf_lambert.h"
#include "brdf_phong.h"
#include "brdf_blinn.h"

namespace XT {
	namespace Render {

/* Bidirectional Scattering Distribution Function */
class BSDF
{
	public:
	virtual ~BSDF();

	virtual const char *get_name() const = 0;

	/*
	* evaluate the radiant exitance along "outdir", of light coming from "indir"
	* returns the intensity for each color channel
	*/
//	virtual nimg::ColorRGBf evaluate(const SurfPoint &pt, const Vector3 &outdir, const Vector3 &indir) const = 0;

	/* evaluate the percentage of incident energy arriving from indir, which
     * ends up reflecting towards outdir (this is just the mean of eval()'s RGB
     * values).
    */
//    virtual double eval_energy(const SurfPoint &pt, const Vector3 &outdir, const Vector3 &indir) const;

	// generate a random BRDF sampling direction
//    virtual Vector3 sample_dir(const SurfPoint &pt, const Vector3 &outdir) const = 0;
};

	}
}

#endif /* XTCORE_BRDF_H_INCLUDED */
