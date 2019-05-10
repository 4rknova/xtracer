#include "integrator.h"

#if FEATURE_IS_INCLUDED(FEATURE_EMBREE)

#include <nmath/precision.h>
#include <nmath/mutil.h>
#include <nmath/prng.h>
#include <nmath/sample.h>
#include <xtcore/math/plane.h>
#include <nimg/luminance.h>
#include <ncf/util.h>
#include <xtcore/tile.h>
#include <xtcore/aa.h>

namespace xtcore {
    namespace integrator {
        namespace embree {

void Integrator::setup_auxiliary()
{
    // create Embree device at application startup
    m_device = rtcNewDevice(0);
    // create scene
    m_scene = rtcNewScene(m_device);
}

void Integrator::clean_auxiliary()
{
    // release objects
    rtcReleaseScene(m_scene);
    rtcReleaseDevice(m_device);
}

void Integrator::render_tile(xtcore::render::tile_t *tile)
{
    xtcore::asset::ICamera *cam = ctx->scene.get_camera(ctx->params.camera);

    while (tile->samples.count() > 0) {
        xtcore::antialiasing::sample_rgba_t sample;
        tile->samples.pop(sample);

		nimg::ColorRGBAf color_pixel;
        nimg::ColorRGBf  color_sample;

        tile->read(sample.pixel.x, sample.pixel.y, color_pixel);

	    xtcore::hit_record_t hit_record;

        xtcore::Ray ray = cam->get_primary_ray(
              sample.coords.x, sample.coords.y
            , (float)(ctx->params.width)
            , (float)(ctx->params.height)
        );

        float embree = 0.f;

        if (ctx->scene.intersection(ray, hit_record)) {
            embree = 1. / log((ray.origin - hit_record.point).length());
        }

        color_sample += nimg::ColorRGBf(embree, embree, embree) * (1. / ctx->params.samples);
        tile->write(floor(sample.pixel.x), floor(sample.pixel.y), nimg::ColorRGBf(color_pixel) + color_sample * sample.weight);
    }
}

        } /* namespace embree */
    } /* namespace integrator */
} /* namespace xtcore */

#endif /* FEATURE_EMBREE */
