#ifndef XTCORE_GPU_OPENCL_SCENE_SERIALIZER_H_INCLUDED
#define XTCORE_GPU_OPENCL_SCENE_SERIALIZER_H_INCLUDED

#ifdef XTCORE_ENABLE_OPENCL

#include <vector>
#include <unordered_map>
#include <cstdint>

#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_TARGET_OPENCL_VERSION  120
#include <CL/opencl.hpp>

#include <xtcore/context.h>
#include "gpu_types.h"

namespace xtcore {
    namespace integrator {
        namespace gpu_opencl {

struct SceneSerializer
{
    gpu_scene_header_t header;
    gpu_camera_t       camera;

    std::vector<gpu_tlas_node_t>    tlas_nodes;
    std::vector<gpu_tlas_item_t>    tlas_items;
    std::vector<gpu_infinite_obj_t> infinite;

    std::vector<gpu_bvh_node_t>  bvh_nodes;
    std::vector<gpu_triangle_t>  triangles;

    std::vector<gpu_sphere_t>  spheres;
    std::vector<gpu_plane_t>   planes;
    std::vector<gpu_fractal_t> fractals;

    std::vector<gpu_object_t>    objects;
    std::vector<gpu_material_t>  materials;
    std::vector<gpu_emissive_t>  emissives;
    std::vector<gpu_tex_desc_t>  tex_descs;
    std::vector<cl_float4>       pixels;
    std::vector<gpu_sampler_t>   samplers;

    void serialize(xtcore::render::context_t &ctx);

private:
    std::unordered_map<const xtcore::asset::IMaterial *, uint32_t> m_mat_index;
    std::unordered_map<uint64_t, uint32_t>                         m_obj_index;

    uint32_t get_or_add_material(const xtcore::asset::IMaterial *mat);
    uint32_t add_sampler(const xtcore::sampler::ISampler *s);
    uint32_t add_texture_desc(const xtcore::sampler::Texture2D *tex);
    void     serialize_object(uint64_t obj_id, const xtcore::asset::Object *obj);
    void     serialize_camera(xtcore::render::context_t &ctx);
};

        } /* namespace gpu_opencl */
    } /* namespace integrator */
} /* namespace xtcore */

#endif /* XTCORE_ENABLE_OPENCL */
#endif /* XTCORE_GPU_OPENCL_SCENE_SERIALIZER_H_INCLUDED */
