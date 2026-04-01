#include <cstdio>
#include <fstream>
#include <string>

#include <xtcore/parseutil.h>
#include <xtcore/scene.h>
#include <xtcore/medium.h>
#include <xtcore/strpool.h>
#include <xtcore/sampler/sampler_rayleigh_sky.h>
#include <xtcore/xtcore.h>

namespace {

int fail(const char *msg)
{
    std::fprintf(stderr, "object_medium_parse_test: %s\n", msg);
    return 1;
}

bool write_text(const std::string &path, const std::string &content)
{
    std::ofstream out(path.c_str(), std::ios::binary | std::ios::trunc);
    if (!out.good()) return false;
    out << content;
    return out.good();
}

} // namespace

int main()
{
    xtcore::init();

    const std::string path = "/tmp/xtracer_object_medium_parse_test.scn";
    const std::string scene_text =
        "title = Object Medium Parse Test\n"
        "description = parser smoke test for object.medium\n"
        "version = 1.0\n"
        "default_camera = cam\n"
        "environment = { type = color, config = { value = col3(0,0,0) } }\n"
        "camera = {\n"
        "  cam = {\n"
        "    type = thin-lens\n"
        "    fov = 45\n"
        "    position = vec3(0,0,-4)\n"
        "    target = vec3(0,0,0)\n"
        "    up = vec3(0,1,0)\n"
        "  }\n"
        "}\n"
        "geometry = {\n"
        "  vol = { type = sphere, position = vec3(0,0,0), radius = 1.0 }\n"
        "  vol_noise = { type = sphere, position = vec3(2,0,0), radius = 1.0 }\n"
        "}\n"
        "material = {\n"
        "  shell = { type = boundary }\n"
        "}\n"
        "medium = {\n"
        "  fog_medium = {\n"
        "    type = homogeneous\n"
        "    sigma_a = col3(0.02,0.03,0.04)\n"
        "    sigma_s = col3(0.10,0.11,0.12)\n"
        "    g = 0.25\n"
        "    emission = col3(0.01,0.00,0.00)\n"
        "  }\n"
        "  fog_noise = {\n"
        "    type = heterogeneous_noise\n"
        "    sigma_a = col3(0.01,0.01,0.01)\n"
        "    sigma_s = col3(0.18,0.20,0.22)\n"
        "    g = 0.15\n"
        "    emission = col3(0.00,0.00,0.00)\n"
        "    density = 1.5\n"
        "    noise_scale = 0.8\n"
        "    noise_min = 0.2\n"
        "    noise_max = 1.0\n"
        "    octaves = 4\n"
        "    lacunarity = 2.0\n"
        "    gain = 0.5\n"
        "    seed = 17\n"
        "  }\n"
        "}\n"
        "object = {\n"
        "  fog = {\n"
        "    geometry = vol\n"
        "    material = shell\n"
        "    medium = fog_medium\n"
        "  }\n"
        "  fog_noise = {\n"
        "    geometry = vol_noise\n"
        "    material = shell\n"
        "    medium = fog_noise\n"
        "  }\n"
        "}\n";

    const std::string inline_path = "/tmp/xtracer_object_medium_inline_reject_test.scn";
    const std::string sky_path = "/tmp/xtracer_rayleigh_sky_parse_test.scn";
    const std::string inline_scene_text =
        "title = Object Medium Inline Reject Test\n"
        "description = inline medium blocks must be rejected\n"
        "version = 1.0\n"
        "default_camera = cam\n"
        "environment = { type = color, config = { value = col3(0,0,0) } }\n"
        "camera = {\n"
        "  cam = {\n"
        "    type = thin-lens\n"
        "    fov = 45\n"
        "    position = vec3(0,0,-4)\n"
        "    target = vec3(0,0,0)\n"
        "    up = vec3(0,1,0)\n"
        "  }\n"
        "}\n"
        "geometry = {\n"
        "  vol = { type = sphere, position = vec3(0,0,0), radius = 1.0 }\n"
        "}\n"
        "material = {\n"
        "  shell = { type = boundary }\n"
        "}\n"
        "object = {\n"
        "  fog = {\n"
        "    geometry = vol\n"
        "    material = shell\n"
        "    medium = {\n"
        "      type = homogeneous\n"
        "      sigma_a = col3(0.02,0.03,0.04)\n"
        "      sigma_s = col3(0.10,0.11,0.12)\n"
        "      g = 0.25\n"
        "      emission = col3(0.01,0.00,0.00)\n"
        "    }\n"
        "  }\n"
        "}\n";

    const std::string sky_scene_text =
        "title = Rayleigh Sky Parse Test\n"
        "description = parser smoke test for rayleigh_sky environment\n"
        "version = 1.0\n"
        "default_camera = cam\n"
        "environment = {\n"
        "  type = rayleigh_sky\n"
        "  config = {\n"
        "    sun_direction = vec3(0.1,0.9,0.2)\n"
        "    sun_intensity = col3(18,16,12)\n"
        "    beta_rayleigh = col3(0.15,0.32,0.74)\n"
        "    ground_color = col3(0.03,0.025,0.02)\n"
        "    density = 1.25\n"
        "    horizon_falloff = 1.8\n"
        "    sun_disk_radius = 1.1\n"
        "    sun_disk_intensity = 1.4\n"
        "    sun_glow_radius = 10.0\n"
        "    sun_glow_intensity = 0.45\n"
        "    sun_glow_falloff = 3.0\n"
        "  }\n"
        "}\n"
        "camera = {\n"
        "  cam = {\n"
        "    type = thin-lens\n"
        "    fov = 45\n"
        "    position = vec3(0,0,-4)\n"
        "    target = vec3(0,0,0)\n"
        "    up = vec3(0,1,0)\n"
        "  }\n"
        "}\n"
        "geometry = {\n"
        "  vol = { type = sphere, position = vec3(0,0,0), radius = 1.0 }\n"
        "}\n"
        "material = {\n"
        "  shell = { type = boundary }\n"
        "}\n"
        "object = {\n"
        "  fog = {\n"
        "    geometry = vol\n"
        "    material = shell\n"
        "  }\n"
        "}\n";

    if (!write_text(path, scene_text)) return fail("failed to write temporary scene");
    if (!write_text(inline_path, inline_scene_text)) return fail("failed to write temporary inline scene");
    if (!write_text(sky_path, sky_scene_text)) return fail("failed to write temporary rayleigh scene");

    {
        xtcore::Scene scene;
        const int rc = xtcore::io::scn::load(&scene, path.c_str(), nullptr, nullptr);
        if (rc != 0) return fail("scene load failed");

        const HASH_UINT64 fog_id = xtcore::pool::str::add("fog");
        if (!scene.has_object_medium(fog_id)) return fail("expected object medium not found");

        const xtcore::asset::medium::IMedium *m = scene.get_object_medium(fog_id);
        if (!m) return fail("medium lookup returned null");
        const xtcore::asset::medium::Homogeneous *h = dynamic_cast<const xtcore::asset::medium::Homogeneous *>(m);
        if (!h) return fail("medium was not parsed as homogeneous implementation");
        if (h->asymmetry() < 0.24f || h->asymmetry() > 0.26f) return fail("unexpected anisotropy value");

        const HASH_UINT64 fog_noise_id = xtcore::pool::str::add("fog_noise");
        if (!scene.has_object_medium(fog_noise_id)) return fail("expected heterogeneous medium not found");
        const xtcore::asset::medium::IMedium *m2 = scene.get_object_medium(fog_noise_id);
        if (!m2) return fail("heterogeneous medium lookup returned null");
        const xtcore::asset::medium::HeterogeneousNoise *hn = dynamic_cast<const xtcore::asset::medium::HeterogeneousNoise *>(m2);
        if (!hn) return fail("medium was not parsed as heterogeneous_noise implementation");
    }

    {
        xtcore::Scene scene_inline;
        const int inline_rc = xtcore::io::scn::load(&scene_inline, inline_path.c_str(), nullptr, nullptr);
        if (inline_rc == 0) return fail("inline medium block should be rejected");
    }

    {
        xtcore::Scene scene_sky;
        const int sky_rc = xtcore::io::scn::load(&scene_sky, sky_path.c_str(), nullptr, nullptr);
        if (sky_rc != 0) return fail("rayleigh sky scene load failed");

        const xtcore::sampler::RayleighSky *sky = dynamic_cast<const xtcore::sampler::RayleighSky *>(scene_sky.m_environment);
        if (!sky) return fail("environment was not parsed as rayleigh sky");
        if (sky->density < 1.24f || sky->density > 1.26f) return fail("unexpected rayleigh sky density");
        if (sky->sun_disk_radius < 1.09f || sky->sun_disk_radius > 1.11f) return fail("unexpected rayleigh sky sun disk radius");

        const nimg::ColorRGBf sample = scene_sky.sample_environment(nmath::Vector3f(0.0f, 1.0f, 0.0f));
        if (sample.r() <= 0.0f || sample.g() <= 0.0f || sample.b() <= 0.0f) return fail("rayleigh sky sample was not positive");
    }

    xtcore::deinit();
    std::printf("object_medium_parse_test: ok\n");
    return 0;
}
