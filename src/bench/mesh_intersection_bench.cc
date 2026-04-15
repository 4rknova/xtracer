#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <nmesh/extras.h>
#include <nmesh/transform.h>

#include <xtcore/camera/perspective.h>
#include <xtcore/log.h>
#include <xtcore/math/hitrecord.h>
#include <xtcore/mesh.h>
#include <xtcore/object.h>
#include <xtcore/scene.h>

namespace {

using bench_clock_t = std::chrono::high_resolution_clock;

static const HASH_UINT64 kBenchCameraId = 1ULL;
static const HASH_UINT64 kBenchSurfaceId = 2ULL;
static const HASH_UINT64 kBenchObjectId = 3ULL;

struct bench_config_t
{
    size_t resolution;
    size_t width;
    size_t height;
    size_t passes;
};

void parse_args(int argc, char **argv, bench_config_t &cfg)
{
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--resolution") == 0 && (i + 1) < argc) {
            cfg.resolution = (size_t)std::strtoull(argv[++i], NULL, 10);
        } else if (std::strcmp(argv[i], "--width") == 0 && (i + 1) < argc) {
            cfg.width = (size_t)std::strtoull(argv[++i], NULL, 10);
        } else if (std::strcmp(argv[i], "--height") == 0 && (i + 1) < argc) {
            cfg.height = (size_t)std::strtoull(argv[++i], NULL, 10);
        } else if (std::strcmp(argv[i], "--passes") == 0 && (i + 1) < argc) {
            cfg.passes = (size_t)std::strtoull(argv[++i], NULL, 10);
        }
    }
}

xtcore::camera::Perspective *setup_camera(xtcore::Scene &scene)
{
    xtcore::camera::Perspective *cam = new xtcore::camera::Perspective();
    cam->position = nmath::Vector3f(0.0f, 0.0f, 0.0f);
    cam->target = nmath::Vector3f(0.0f, 0.0f, 1.0f);
    cam->up = nmath::Vector3f(0.0f, 1.0f, 0.0f);
    cam->fov = 45.0f;
    scene.m_cameras[kBenchCameraId] = cam;
    return cam;
}

void setup_mesh_scene(xtcore::Scene &scene, size_t resolution)
{
    nmesh::object_t obj;
    nmesh::generator::icosphere(&obj, resolution);
    nmesh::mutator::translate(obj, 0.0f, 0.0f, 4.0f);

    xtcore::surface::Mesh *mesh = new xtcore::surface::Mesh();
    mesh->build_bvh(obj);

    xtcore::asset::Object *surface_ref = new xtcore::asset::Object();
    surface_ref->surface = kBenchSurfaceId;
    surface_ref->material = 0;

    scene.m_surface[kBenchSurfaceId] = mesh;
    scene.m_objects[kBenchObjectId] = surface_ref;
}

std::vector<xtcore::Ray> build_primary_rays(xtcore::camera::Perspective *cam, size_t width, size_t height)
{
    std::vector<xtcore::Ray> rays;
    rays.reserve(width * height);
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            rays.push_back(cam->get_primary_ray(
                static_cast<float>(x) + 0.5f,
                static_cast<float>(y) + 0.5f,
                static_cast<float>(width),
                static_cast<float>(height)));
        }
    }
    return rays;
}

} // namespace

int main(int argc, char **argv)
{
    bench_config_t cfg;
    cfg.resolution = 96;
    cfg.width = 320;
    cfg.height = 320;
    cfg.passes = 12;
    parse_args(argc, argv, cfg);

    xtcore::reset_mesh_bvh_stats();

    xtcore::Scene scene;
    xtcore::camera::Perspective *cam = setup_camera(scene);
    setup_mesh_scene(scene, cfg.resolution);
    scene.rebuild_spatial_index();

    const std::vector<xtcore::Ray> rays = build_primary_rays(cam, cfg.width, cfg.height);
    const xtcore::spatial_index_stats_t spatial_stats = xtcore::get_last_spatial_index_stats();
    const xtcore::mesh_bvh_stats_t build_stats = xtcore::get_last_mesh_bvh_stats();

    xtcore::reset_mesh_bvh_stats();

    unsigned long long hits = 0ULL;
    double t_sum = 0.0;
    const auto t0 = bench_clock_t::now();
    for (size_t pass = 0; pass < cfg.passes; ++pass) {
        for (size_t i = 0; i < rays.size(); ++i) {
            xtcore::hit_record_t hit;
            if (scene.intersection(rays[i], hit)) {
                hits += 1ULL;
                t_sum += (double)hit.t;
            }
        }
    }
    const auto t1 = bench_clock_t::now();

    const double elapsed_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    const double rays_total = (double)(rays.size() * cfg.passes);
    const double mrays_per_sec = rays_total / (elapsed_ms * 1000.0);

    const xtcore::mesh_bvh_stats_t trace_stats = xtcore::get_last_mesh_bvh_stats();

    const double avg_nodes_per_ray =
        trace_stats.rays_tested ? (double)trace_stats.nodes_visited / (double)trace_stats.rays_tested : 0.0;
    const double avg_triangles_per_ray =
        trace_stats.rays_tested ? (double)trace_stats.triangle_tests / (double)trace_stats.rays_tested : 0.0;
    const double avg_t_on_hit = hits ? t_sum / (double)hits : 0.0;

    xtcore::Log::handle().post_message("mesh intersection bench");
    xtcore::Log::handle().post_message("config                 : resolution=%zu width=%zu height=%zu passes=%zu",
                                       cfg.resolution, cfg.width, cfg.height, cfg.passes);
    xtcore::Log::handle().post_message("mesh build             : triangles=%zu nodes=%zu leaves=%zu build_ms=%.3f",
                                       build_stats.triangles, build_stats.bvh_nodes, build_stats.bvh_leaves, build_stats.build_ms);
    xtcore::Log::handle().post_message("scene tlas             : nodes=%zu leaves=%zu build_ms=%.3f",
                                       spatial_stats.tlas_nodes, spatial_stats.tlas_leaves, spatial_stats.build_ms);
    xtcore::Log::handle().post_message("trace                  : %10.3f ms  (%8.3f Mray/s)", elapsed_ms, mrays_per_sec);
    xtcore::Log::handle().post_message("hits                   : %10llu", hits);
    xtcore::Log::handle().post_message("avg nodes/ray          : %10.3f", avg_nodes_per_ray);
    xtcore::Log::handle().post_message("avg triangles/ray      : %10.3f", avg_triangles_per_ray);
    xtcore::Log::handle().post_message("avg t on hit           : %10.6f", avg_t_on_hit);
    xtcore::Log::handle().post_message("metric|trace_ms|%.6f", elapsed_ms);
    xtcore::Log::handle().post_message("metric|mrays_per_sec|%.6f", mrays_per_sec);
    xtcore::Log::handle().post_message("metric|avg_nodes_per_ray|%.6f", avg_nodes_per_ray);
    xtcore::Log::handle().post_message("metric|avg_triangles_per_ray|%.6f", avg_triangles_per_ray);
    xtcore::Log::handle().post_message("metric|hits|%llu", hits);
    return 0;
}
