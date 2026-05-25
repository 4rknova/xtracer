#ifdef XTCORE_ENABLE_OPENCL

#include <cstdio>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <nimg/pixmap.h>

#include "pathtracer_cl_src.h"
#include "gpu_types.h"
#include "scene_serializer.h"
#include "integrator.h"

namespace xtcore {
    namespace integrator {
        namespace gpu_opencl {

/* ---- Kernel binary cache ---- */

static uint64_t fnv1a(const char *s, size_t len)
{
    uint64_t h = 14695981039346656037ULL;
    for (size_t i = 0; i < len; ++i) { h ^= (uint8_t)s[i]; h *= 1099511628211ULL; }
    return h;
}

static std::string kernel_cache_path(const std::string &device_name)
{
    const char *src = k_opencl_pathtracer_src;
    uint64_t h = fnv1a(src, strlen(src));
    for (char c : device_name) { h ^= (uint8_t)c; h *= 1099511628211ULL; }
    char path[512];
    const char *tmp = getenv("TMPDIR");
    if (!tmp || !tmp[0]) tmp = "/tmp";
    snprintf(path, sizeof(path), "%s/xtracer_ocl_%016llx.bin", tmp,
             (unsigned long long)h);
    return path;
}

/* ---- Integrator ---- */

Integrator::Integrator()
    : m_gpu_ready(false)
    , m_samples_dispatched(0)
    , m_header()
    , m_camera()
{}

Integrator::~Integrator()
{
    cleanup_gpu();
}

xtcore::render::integrator_metadata_t Integrator::metadata() const
{
    xtcore::render::integrator_metadata_t meta;
    meta.id               = "gpu_opencl";
    meta.name             = "GPU Path Tracer (OpenCL)";
    meta.status           = xtcore::render::INTEGRATOR_STATUS_EXPERIMENTAL;
    meta.description      = "Experimental OpenCL GPU path tracer.";
    meta.uses_cpu_threads = false;
    return meta;
}


void Integrator::setup_gpu()
{
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    if (platforms.empty()) throw std::runtime_error("No OpenCL platforms found");

    cl::Device device;
    bool found = false;
    for (auto &plat : platforms) {
        std::vector<cl::Device> devices;
        plat.getDevices(CL_DEVICE_TYPE_GPU, &devices);
        if (!devices.empty()) { device = devices[0]; found = true; break; }
    }
    if (!found) {
        for (auto &plat : platforms) {
            std::vector<cl::Device> devices;
            plat.getDevices(CL_DEVICE_TYPE_ALL, &devices);
            if (!devices.empty()) { device = devices[0]; found = true; break; }
        }
    }
    if (!found) throw std::runtime_error("No OpenCL device found");

    m_context = cl::Context(device);
    m_queue   = cl::CommandQueue(m_context, device);

    std::string device_name = device.getInfo<CL_DEVICE_NAME>();
    std::string cache_path  = kernel_cache_path(device_name);

    /* Try to load pre-compiled binary. */
    bool loaded_from_cache = false;
    {
        std::ifstream f(cache_path, std::ios::binary | std::ios::ate);
        if (f) {
            std::streamsize sz = f.tellg();
            if (sz > 0) {
                f.seekg(0);
                std::vector<unsigned char> binary((size_t)sz);
                f.read(reinterpret_cast<char*>(binary.data()), sz);
                try {
                    cl::Program::Binaries bins = { binary };
                    std::vector<cl::Device> devs = { device };
                    m_program = cl::Program(m_context, devs, bins);
                    cl_int build_err = m_program.build({device});
                    if (build_err == CL_SUCCESS) {
                        loaded_from_cache = true;
                        fprintf(stderr, "[gpu_opencl] Loaded kernel binary from cache (%s)\n",
                                cache_path.c_str());
                    }
                } catch (...) {
                    /* Cache corrupt or for a different driver version — fall through. */
                }
            }
        }
    }

    if (!loaded_from_cache) {
        fprintf(stderr, "[gpu_opencl] Compiling kernel (this may take a few seconds)...\n");
        cl::Program::Sources sources;
        sources.push_back(k_opencl_pathtracer_src);
        m_program = cl::Program(m_context, sources);

        auto t0 = std::chrono::steady_clock::now();
        cl_int build_err = m_program.build({device});
        double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();

        std::string build_log = m_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
        if (!build_log.empty() && build_log != "\n")
            fprintf(stderr, "[gpu_opencl] Build log:\n%s\n", build_log.c_str());
        if (build_err != CL_SUCCESS)
            throw std::runtime_error("OpenCL kernel build failed");
        fprintf(stderr, "[gpu_opencl] Kernel compiled in %.1fs\n", elapsed);

        /* Save binary to cache for next run. */
        try {
            auto binaries = m_program.getInfo<CL_PROGRAM_BINARIES>();
            if (!binaries.empty() && !binaries[0].empty()) {
                std::ofstream f(cache_path, std::ios::binary);
                const auto &bin = binaries[0];
                f.write(reinterpret_cast<const char*>(bin.data()), bin.size());
                if (f) fprintf(stderr, "[gpu_opencl] Kernel binary cached (%s)\n",
                               cache_path.c_str());
            }
        } catch (...) { /* Non-fatal — next run will recompile. */ }
    }

    m_kernel = cl::Kernel(m_program, "pathtrace");
}

void Integrator::upload_scene()
{
    if (!ctx || !ctx->scene) return;

    ctx->scene->rebuild_spatial_index();

    SceneSerializer ser;
    ser.serialize(*ctx);

    m_header = ser.header;
    m_camera = ser.camera;

    auto upload = [&](cl::Buffer &buf, const void *data, size_t bytes) {
        if (bytes == 0) bytes = 4;
        buf = cl::Buffer(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, bytes,
                         const_cast<void*>(data));
    };

    upload(m_buf_tlas_nodes, ser.tlas_nodes.data(),   ser.tlas_nodes.size()   * sizeof(gpu_tlas_node_t));
    upload(m_buf_tlas_items, ser.tlas_items.data(),   ser.tlas_items.size()   * sizeof(gpu_tlas_item_t));
    upload(m_buf_infinite,   ser.infinite.data(),     ser.infinite.size()     * sizeof(gpu_infinite_obj_t));
    upload(m_buf_bvh_nodes,  ser.bvh_nodes.data(),    ser.bvh_nodes.size()    * sizeof(gpu_bvh_node_t));
    upload(m_buf_triangles,  ser.triangles.data(),    ser.triangles.size()    * sizeof(gpu_triangle_t));
    upload(m_buf_spheres,    ser.spheres.data(),      ser.spheres.size()      * sizeof(gpu_sphere_t));
    upload(m_buf_planes,     ser.planes.data(),       ser.planes.size()       * sizeof(gpu_plane_t));
    upload(m_buf_fractals,   ser.fractals.data(),     ser.fractals.size()     * sizeof(gpu_fractal_t));
    upload(m_buf_objects,    ser.objects.data(),      ser.objects.size()      * sizeof(gpu_object_t));
    upload(m_buf_materials,  ser.materials.data(),    ser.materials.size()    * sizeof(gpu_material_t));
    upload(m_buf_emissives,  ser.emissives.data(),    ser.emissives.size()    * sizeof(gpu_emissive_t));
    upload(m_buf_tex_descs,  ser.tex_descs.data(),    ser.tex_descs.size()    * sizeof(gpu_tex_desc_t));
    upload(m_buf_pixels,     ser.pixels.data(),       ser.pixels.size()       * sizeof(cl_float4));
    upload(m_buf_samplers,   ser.samplers.data(),     ser.samplers.size()     * sizeof(gpu_sampler_t));
}

void Integrator::cleanup_gpu()
{
    m_buf_tlas_nodes = cl::Buffer();
    m_buf_tlas_items = cl::Buffer();
    m_buf_infinite   = cl::Buffer();
    m_buf_bvh_nodes  = cl::Buffer();
    m_buf_triangles  = cl::Buffer();
    m_buf_spheres    = cl::Buffer();
    m_buf_planes     = cl::Buffer();
    m_buf_fractals   = cl::Buffer();
    m_buf_objects    = cl::Buffer();
    m_buf_materials  = cl::Buffer();
    m_buf_emissives  = cl::Buffer();
    m_buf_tex_descs  = cl::Buffer();
    m_buf_pixels     = cl::Buffer();
    m_buf_samplers   = cl::Buffer();
    m_buf_accum      = cl::Buffer();
    m_gpu_ready      = false;
}

void Integrator::render()
{
    if (!ctx) return;

    const size_t width  = ctx->params.width;
    const size_t height = ctx->params.height;
    if (width == 0 || height == 0) return;

    /* One-time setup: compile kernel (binary cache) + upload scene. */
    if (!m_gpu_ready) {
        try {
            setup_gpu();
            upload_scene();
        } catch (const std::exception &e) {
            fprintf(stderr, "[gpu_opencl] Setup failed: %s\n", e.what());
            cleanup_gpu();
            return;
        }

        fprintf(stderr, "[gpu_opencl] scene: %u objects, %u fractals, %u tlas_nodes, %u tlas_items, %u infinite\n",
                m_header.object_count, m_header.fractal_count,
                m_header.tlas_node_count, m_header.tlas_item_count, m_header.infinite_count);
        fprintf(stderr, "[gpu_opencl] camera pos=(%.3f,%.3f,%.3f) fwd=(%.3f,%.3f,%.3f) fov_w=%.3f fov_h=%.3f\n",
                m_camera.position.x, m_camera.position.y, m_camera.position.z,
                m_camera.forward.x,  m_camera.forward.y,  m_camera.forward.z,
                m_camera.half_w_over_d, m_camera.half_h_over_d);

        const size_t accum_bytes = width * height * sizeof(cl_float4);
        m_buf_accum = cl::Buffer(m_context, CL_MEM_READ_WRITE, accum_bytes);

        m_gpu_ready = true;
    }

    for (auto &tile : ctx->tiles) tile.init();

    std::vector<cl_float4> host_accum(width * height);

    const int samples   = (int)ctx->params.samples;
    const int max_depth = (int)ctx->params.rdepth;
    const cl_uint aa    = (cl_uint)std::max((size_t)1, ctx->params.aa);

    {
        int arg = 0;
        m_kernel.setArg(arg++, m_header);
        m_kernel.setArg(arg++, m_buf_tlas_nodes);
        m_kernel.setArg(arg++, m_buf_tlas_items);
        m_kernel.setArg(arg++, m_buf_infinite);
        m_kernel.setArg(arg++, m_buf_bvh_nodes);
        m_kernel.setArg(arg++, m_buf_triangles);
        m_kernel.setArg(arg++, m_buf_spheres);
        m_kernel.setArg(arg++, m_buf_planes);
        m_kernel.setArg(arg++, m_buf_fractals);
        m_kernel.setArg(arg++, m_buf_objects);
        m_kernel.setArg(arg++, m_buf_materials);
        m_kernel.setArg(arg++, m_buf_tex_descs);
        m_kernel.setArg(arg++, m_buf_pixels);
        m_kernel.setArg(arg++, m_camera);
        m_kernel.setArg(arg++, (cl_uint)m_samples_dispatched); // sample_offset
        m_kernel.setArg(arg++, (cl_uint)max_depth);
        m_kernel.setArg(arg++, m_buf_accum);
        m_kernel.setArg(arg++, (cl_uint)width);
        m_kernel.setArg(arg++, (cl_uint)height);
        m_kernel.setArg(arg++, aa);
        m_kernel.setArg(arg++, m_buf_emissives);
        m_kernel.setArg(arg++, m_buf_samplers);
        m_kernel.setArg(arg++, (cl_uint)samples);
    }

    bool dispatch_ok = false;
    if (!should_abort()) {
        try {
            m_queue.enqueueNDRangeKernel(m_kernel, cl::NullRange,
                                         cl::NDRange(width, height), cl::NullRange);
            m_queue.finish();
            dispatch_ok = true;
        } catch (const std::exception &e) {
            fprintf(stderr, "[gpu_opencl] Kernel dispatch error: %s\n", e.what());
        }
    }

    m_samples_dispatched += (uint32_t)samples;

    if (dispatch_ok) {
        const size_t accum_bytes = width * height * sizeof(cl_float4);
        m_queue.enqueueReadBuffer(m_buf_accum, CL_TRUE, 0, accum_bytes, host_accum.data());
        for (auto &tile : ctx->tiles) {
            for (size_t y = tile.y0(); y < tile.y1(); ++y) {
                for (size_t x = tile.x0(); x < tile.x1(); ++x) {
                    const cl_float4 &acc = host_accum[y * width + x];
                    tile.write(x, y, nimg::ColorRGBAf(acc.s[0], acc.s[1], acc.s[2], 1.0f));
                }
            }
            tile.submit();
        }
    }
    /* GPU state persists for the next render() call on this instance. */
}

        } /* namespace gpu_opencl */
    } /* namespace integrator */
} /* namespace xtcore */

#endif /* XTCORE_ENABLE_OPENCL */
