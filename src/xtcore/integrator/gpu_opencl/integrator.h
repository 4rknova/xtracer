#ifndef XTCORE_INTEGRATOR_GPU_OPENCL_H_INCLUDED
#define XTCORE_INTEGRATOR_GPU_OPENCL_H_INCLUDED

#ifdef XTCORE_ENABLE_OPENCL

#include <vector>

#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_TARGET_OPENCL_VERSION  120
#define CL_HPP_ENABLE_EXCEPTIONS
#include <CL/opencl.hpp>

#include <xtcore/integrator.h>
#include "gpu_types.h"

namespace xtcore {
    namespace integrator {
        namespace gpu_opencl {

class Integrator : public xtcore::render::IIntegrator
{
public:
    Integrator();
    ~Integrator();

    virtual xtcore::render::integrator_metadata_t metadata() const;
    virtual void render() override;
    virtual void render_tile(xtcore::render::tile_t *) override {}

private:
    void setup_gpu();
    void upload_scene();
    void cleanup_gpu();

    cl::Context      m_context;
    cl::CommandQueue m_queue;
    cl::Program      m_program;
    cl::Kernel       m_kernel;

    cl::Buffer m_buf_tlas_nodes;
    cl::Buffer m_buf_tlas_items;
    cl::Buffer m_buf_infinite;
    cl::Buffer m_buf_bvh_nodes;
    cl::Buffer m_buf_triangles;
    cl::Buffer m_buf_spheres;
    cl::Buffer m_buf_planes;
    cl::Buffer m_buf_fractals;
    cl::Buffer m_buf_objects;
    cl::Buffer m_buf_materials;
    cl::Buffer m_buf_emissives;
    cl::Buffer m_buf_tex_descs;
    cl::Buffer m_buf_pixels;
    cl::Buffer m_buf_samplers;
    cl::Buffer m_buf_accum;

    bool     m_gpu_ready;
    uint32_t m_samples_dispatched; /* cumulative across render() calls on same instance */

    gpu_scene_header_t m_header;
    gpu_camera_t       m_camera;
};

        } /* namespace gpu_opencl */
    } /* namespace integrator */
} /* namespace xtcore */

#endif /* XTCORE_ENABLE_OPENCL */
#endif /* XTCORE_INTEGRATOR_GPU_OPENCL_H_INCLUDED */
