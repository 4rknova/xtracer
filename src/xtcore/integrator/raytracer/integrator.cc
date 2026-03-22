#include <cmath>
#include <vector>

#include <nmath/precision.h>
#include <nimg/luminance.h>
#include <xtcore/aa.h>
#include <xtcore/matdefs.h>

#include "integrator.h"

namespace xtcore {
    namespace integrator {
        namespace raytracer {

namespace {

bool visible_light_sample(
    xtcore::Scene &scene,
    const xtcore::hit_record_t &hit_record,
    const xtcore::light_t &light,
    xtcore::asset::emitter_t &out_emitter
)
{
    out_emitter.position = light.light->emitter_position();

    nmath::Vector3f to_light = out_emitter.position - hit_record.point;
    if (to_light.length() <= (nmath::scalar_t)EPSILON) return false;

    xtcore::Ray shadow_ray;
    shadow_ray.origin = hit_record.point + hit_record.normal * EPSILON;
    shadow_ray.direction = to_light.normalized();

    xtcore::hit_record_t occ;
    if (!scene.intersection(shadow_ray, occ)) return false;

    const xtcore::asset::ISurface *occ_surface = scene.get_surface(occ.id_object);
    const xtcore::asset::IMaterial *occ_material = scene.get_material(occ.id_object);
    if (occ_surface != light.light || occ_material != light.material) return false;

    out_emitter.intensity = light.material->get_sample(MAT_SAMPLER_EMISSIVE, occ.texcoord);
    return true;
}

bool finite_color(const nimg::ColorRGBf &c)
{
    return std::isfinite((double)c.r()) && std::isfinite((double)c.g()) && std::isfinite((double)c.b());
}

} // namespace

nimg::ColorRGBf Integrator::eval(size_t depth, hit_result_t &in)
{
    if (depth == 0) return nimg::ColorRGBf(0, 0, 0);

    xtcore::hit_record_t hit_record;
    bool hit = ctx->scene.intersection(in.ray, hit_record);
    hit_record.ior = in.ior;

    if (!hit) {
        return ctx->scene.sample_environment(in.ray.direction);
    }

    const xtcore::asset::IMaterial *mat = ctx->scene.get_material(hit_record.id_object);
    if (!mat) return nimg::ColorRGBf(0, 0, 0);

    // If we directly hit an emissive surface, return its emission immediately.
    // This keeps emissive quads/triangles visible to camera and specular paths.
    if (mat->is_emissive()) {
        const nimg::ColorRGBf emitted = mat->get_sample(MAT_SAMPLER_EMISSIVE, hit_record.texcoord);
        return finite_color(emitted) ? emitted : nimg::ColorRGBf(0, 0, 0);
    }

    nimg::ColorRGBf color(0, 0, 0);

    // Classic Whitted-style direct lighting from visible emissive geometry samples.
    std::vector<xtcore::light_t> lights;
    ctx->scene.get_light_sources(lights);

    xtcore::asset::ICamera *cam = ctx->scene.get_camera(ctx->params.camera);
    if (cam) {
        for (size_t i = 0; i < lights.size(); ++i) {
            const xtcore::light_t &light = lights[i];
            if (!light.light || !light.material) continue;

            xtcore::asset::emitter_t emitter;
            if (!visible_light_sample(ctx->scene, hit_record, light, emitter)) continue;

            nimg::ColorRGBf direct(0, 0, 0);
            mat->shade(direct, cam, &emitter, hit_record);
            color += direct;
        }
    }

    // Strict Whitted behavior: no diffuse GI recursion.
    // Recurse for materials with explicit reflective/transmissive paths.
    // Also recurse for zero-diffuse materials to preserve prior behavior.
    const nimg::ColorRGBf kd = mat->get_sample(MAT_SAMPLER_DIFFUSE, hit_record.texcoord);
    nmath::scalar_t reflectance = (nmath::scalar_t)mat->get_scalar(MAT_SCALART_REFLECTANCE);
    if (reflectance < (nmath::scalar_t)0.0) reflectance = (nmath::scalar_t)0.0;
    if (reflectance > (nmath::scalar_t)1.0) reflectance = (nmath::scalar_t)1.0;

    nmath::scalar_t transparency = (nmath::scalar_t)mat->get_scalar(MAT_SCALART_TRANSPARENCY);
    if (transparency < (nmath::scalar_t)0.0) transparency = (nmath::scalar_t)0.0;
    if (transparency > (nmath::scalar_t)1.0) transparency = (nmath::scalar_t)1.0;

    const bool recurse_path =
           (nimg::eval::luminance(kd) <= (nmath::scalar_t)EPSILON)
        || (reflectance > (nmath::scalar_t)EPSILON)
        || (transparency > (nmath::scalar_t)EPSILON);

    if (recurse_path) {
        xtcore::hit_result_t next;
        next.intensity = nimg::ColorRGBf(1, 1, 1);
        next.ior = in.ior;

        if (mat->sample_path(next, hit_record)) {
            nimg::ColorRGBf reflected = eval(depth - 1, next);
            color += next.intensity * reflected;
        }
    }

    if (!finite_color(color)) return nimg::ColorRGBf(0, 0, 0);
    return color;
}

void Integrator::render_tile(xtcore::render::tile_t *tile)
{
    xtcore::asset::ICamera *cam = ctx->scene.get_camera(ctx->params.camera);

    while (tile->samples.count() > 0) {
        xtcore::antialiasing::sample_rgba_t aa_sample;
        tile->samples.pop(aa_sample);

        nimg::ColorRGBAf color_pixel;
        tile->read(aa_sample.pixel.x, aa_sample.pixel.y, color_pixel);

        hit_result_t hit_result;
        hit_result.intensity = nimg::ColorRGBf(1, 1, 1);
        hit_result.ior = 1.f;
        hit_result.ray = cam->get_primary_ray(
              aa_sample.coords.x
            , aa_sample.coords.y
            , (float)(ctx->params.width)
            , (float)(ctx->params.height)
        );

        color_pixel += eval(ctx->params.rdepth, hit_result) * aa_sample.weight;
        color_pixel.a(1.0f);

        tile->write((size_t)floor(aa_sample.pixel.x), (size_t)floor(aa_sample.pixel.y), color_pixel);
    }
}

        } /* namespace raytracer */
    } /* namespace integrator */
} /* namespace xtcore */
