#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <map>
#include <new>
#include <vector>

#include <ncf/util.h>
#include <cgltf.h>
#include <nmesh/obj.h>
#include <ufbx/ufbx.h>

#include "import_asset.h"
#include "log.h"
#include "mesh.h"
#include "material.h"
#include "matdefs.h"
#include "sampler/sampler_col.h"

namespace xtcore {

namespace {

static bool path_exists(const std::string &path)
{
    if (path.empty()) return false;
    std::ifstream in(path.c_str(), std::ios::binary);
    return in.good();
}

static std::string resolve_import_case_path(const std::string &path)
{
    if (path.empty()) return path;
    if (path_exists(path)) return path;

    const size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) return path;

    std::string ext = path.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
        return (char)std::tolower(c);
    });

    std::vector<std::string> variants;
    variants.push_back(ext);
    std::string upper = ext;
    std::transform(upper.begin(), upper.end(), upper.begin(), [](unsigned char c) {
        return (char)std::toupper(c);
    });
    if (upper != ext) variants.push_back(upper);
    std::string title = ext;
    if (!title.empty()) title[0] = (char)std::toupper((unsigned char)title[0]);
    if (title != ext && title != upper) variants.push_back(title);

    for (size_t i = 0; i < variants.size(); ++i) {
        std::string candidate = path.substr(0, dot + 1);
        candidate.append(variants[i]);
        if (path_exists(candidate)) return candidate;
    }

    return path;
}

static std::string lowercase_extension(const std::string &path)
{
    const size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) return std::string();
    std::string ext = path.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
        return (char)std::tolower(c);
    });
    return ext;
}

static int append_texture(std::vector<imported_texture_t> &textures, const std::string &path)
{
    if (path.empty()) return -1;
    for (size_t i = 0; i < textures.size(); ++i) {
        if (!textures[i].is_embedded && textures[i].path == path) return (int)i;
    }
    imported_texture_t tex;
    tex.path = path;
    tex.name = path;
    textures.push_back(tex);
    return (int)(textures.size() - 1);
}

static std::string to_std_string(ufbx_string str)
{
    if (!str.data || str.length == 0) return std::string();
    return std::string(str.data, str.length);
}

static std::string guess_format_from_path(const std::string &path)
{
    const size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) return std::string();
    std::string ext = path.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
        return (char)std::tolower(c);
    });
    return ext;
}

static std::string sanitize_import_name(const std::string &value, const char *fallback)
{
    if (!value.empty()) return value;
    return fallback ? std::string(fallback) : std::string();
}

static xtcore::sampler::ISampler *create_color_fallback_sampler(const float value[3], bool white_fallback_on_missing)
{
    float r = value ? value[0] : 0.0f;
    float g = value ? value[1] : 0.0f;
    float b = value ? value[2] : 0.0f;
    if (white_fallback_on_missing && r <= 0.0f && g <= 0.0f && b <= 0.0f) {
        r = 1.0f;
        g = 1.0f;
        b = 1.0f;
    }

    xtcore::sampler::SolidColor *sampler = new (std::nothrow) xtcore::sampler::SolidColor();
    if (!sampler) return 0;
    nimg::ColorRGBf color(r, g, b);
    sampler->set(color);
    return sampler;
}

static xtcore::sampler::ISampler *create_texture_sampler(const imported_asset_t &asset,
                                                         int texture_index,
                                                         const float value[3],
                                                         bool white_fallback_on_missing)
{
    if (texture_index >= 0 && (size_t)texture_index < asset.textures.size()) {
        const imported_texture_t &src = asset.textures[(size_t)texture_index];
        xtcore::sampler::Texture2D *tex = new (std::nothrow) xtcore::sampler::Texture2D();
        if (tex && src.is_embedded && !src.embedded_bytes.empty()) {
            if (tex->load_memory(src.embedded_bytes.data(), src.embedded_bytes.size()) == 0) {
                tex->set_filtering(xtcore::sampler::FILTERING_BILINEAR);
                tex->flip_vertical();
                return tex;
            }
            Log::handle().post_warning("Failed to load embedded imported texture %s, trying file path fallback",
                                       src.name.c_str());
        }

        if (tex && !src.path.empty()) {
            std::string normalized_path = asset.base_dir;
            normalized_path.append(src.path);
            std::replace(normalized_path.begin(), normalized_path.end(), '\\', '/');
            if (tex->load(normalized_path.c_str()) == 0) {
                tex->set_filtering(xtcore::sampler::FILTERING_BILINEAR);
                tex->flip_vertical();
                return tex;
            }
            Log::handle().post_warning("Failed to load imported texture %s, using solid color fallback", normalized_path.c_str());
        }

        if (tex) {
            delete tex;
        }
    }

    return create_color_fallback_sampler(value, white_fallback_on_missing);
}

static xtcore::asset::IMaterial *build_runtime_material(const imported_asset_t &asset, const imported_material_t &src)
{
    const bool has_emissive_color = (src.emissive[0] > 0.0f) || (src.emissive[1] > 0.0f) || (src.emissive[2] > 0.0f);
    const bool has_emissive = has_emissive_color || src.emissive_texture >= 0;
    const bool has_principled = src.roughness_texture >= 0
        || src.metallic_texture >= 0
        || src.roughness > 0.0f
        || src.metallic > 0.0f
        || src.shading_model == imported_material_t::SHADING_PRINCIPLED;

    xtcore::asset::IMaterial *mat = 0;
    if (has_emissive && !has_principled) {
        mat = new (std::nothrow) xtcore::asset::material::Emissive();
        if (!mat) return 0;
        mat->add_sampler(MAT_SAMPLER_EMISSIVE, create_texture_sampler(asset, src.emissive_texture, src.emissive, false));
        return mat;
    }

    if (has_principled) {
        mat = new (std::nothrow) xtcore::asset::material::Principled();
        if (!mat) return 0;
        mat->add_sampler(MAT_SAMPLER_BASE_COLOR, create_texture_sampler(asset, src.base_color_texture, src.base_color, true));
        if (src.normal_texture >= 0) {
            static const float kNormalDefault[3] = { 0.5f, 0.5f, 1.0f };
            mat->add_sampler(MAT_SAMPLER_NORMAL, create_texture_sampler(asset, src.normal_texture, kNormalDefault, false));
        }
        if (src.roughness_texture >= 0) {
            const float rough[3] = { src.roughness, src.roughness, src.roughness };
            mat->add_sampler(MAT_SAMPLER_ROUGHNESS, create_texture_sampler(asset, src.roughness_texture, rough, false));
        }
        if (src.metallic_texture >= 0) {
            const float metal[3] = { src.metallic, src.metallic, src.metallic };
            mat->add_sampler(MAT_SAMPLER_METALLIC, create_texture_sampler(asset, src.metallic_texture, metal, false));
        }
        if (has_emissive) {
            mat->add_sampler(MAT_SAMPLER_EMISSIVE, create_texture_sampler(asset, src.emissive_texture, src.emissive, false));
        }
        mat->add_scalar(MAT_SCALART_ROUGHNESS, src.roughness);
        mat->add_scalar(MAT_SCALART_METALLIC, src.metallic);
        mat->add_scalar(MAT_SCALART_IOR, src.ior);
        return mat;
    }

    mat = new (std::nothrow) xtcore::asset::material::Lambert();
    if (!mat) return 0;
    mat->add_sampler(MAT_SAMPLER_DIFFUSE, create_texture_sampler(asset, src.base_color_texture, src.base_color, true));
    if (src.normal_texture >= 0) {
        static const float kNormalDefault[3] = { 0.5f, 0.5f, 1.0f };
        mat->add_sampler(MAT_SAMPLER_NORMAL, create_texture_sampler(asset, src.normal_texture, kNormalDefault, false));
    }
    return mat;
}

static HASH_UINT64 ensure_fallback_material(xtcore::Scene *scene, const imported_asset_t &asset, const char *prefix)
{
    const std::string base_prefix = prefix ? std::string(prefix) : std::string();
    const std::string fallback_name = base_prefix + "__fallback_material";
    const HASH_UINT64 id = xtcore::pool::str::add(fallback_name.c_str());
    if (scene->m_materials.find(id) != scene->m_materials.end()) return id;

    xtcore::asset::IMaterial *mat = new (std::nothrow) xtcore::asset::material::Lambert();
    if (!mat) return HASH_ID_INVALID;
    const float kd[3] = { 1.0f, 1.0f, 1.0f };
    mat->add_sampler(MAT_SAMPLER_DIFFUSE, create_color_fallback_sampler(kd, false));
    scene->m_materials[id] = mat;
    (void)asset;
    return id;
}

static bool import_asset_obj(const char *path, imported_asset_t &out, std::string &err)
{
    nmesh::object_t obj;

    std::string base, file, source_path = path ? std::string(path) : std::string();
    ncf::util::path_comp(source_path, base, file);
    std::string import_path = resolve_import_case_path(source_path);
    if (import_path != source_path) {
        Log::handle().post_warning("OBJ path case fallback: %s -> %s", source_path.c_str(), import_path.c_str());
    }

    if (nmesh::io::import::obj(import_path.c_str(), obj, base.c_str())) {
        err = "failed to import OBJ asset";
        return false;
    }

    out.source_path = import_path;
    out.base_dir = base;
    out.attributes = obj.attributes;
    out.textures.clear();
    out.materials.clear();
    out.shapes.clear();

    for (size_t i = 0; i < obj.materials.size(); ++i) {
        const nmesh::material_t &src = obj.materials[i];
        imported_material_t dst;
        dst.name = src.name;
        dst.base_color[0] = src.diffuse[0];
        dst.base_color[1] = src.diffuse[1];
        dst.base_color[2] = src.diffuse[2];
        dst.emissive[0] = src.emission[0];
        dst.emissive[1] = src.emission[1];
        dst.emissive[2] = src.emission[2];
        dst.transmission[0] = src.transmittance[0];
        dst.transmission[1] = src.transmittance[1];
        dst.transmission[2] = src.transmittance[2];
        dst.roughness = src.roughness;
        dst.metallic = src.metallic;
        dst.ior = src.ior > 0.0f ? src.ior : 1.0f;
        dst.opacity = src.dissolve;
        dst.base_color_texture = append_texture(out.textures, src.texture_diffuse);
        dst.emissive_texture = append_texture(out.textures, src.texture_emissive);
        dst.normal_texture = append_texture(out.textures, src.texture_normal.empty() ? src.texture_bump : src.texture_normal);
        dst.roughness_texture = append_texture(out.textures, src.texture_roughness);
        dst.metallic_texture = append_texture(out.textures, src.texture_metallic);
        dst.transmission_texture = -1;

        const bool has_emissive = dst.emissive_texture >= 0
            || dst.emissive[0] > 0.0f
            || dst.emissive[1] > 0.0f
            || dst.emissive[2] > 0.0f;
        const bool has_principled = dst.roughness_texture >= 0
            || dst.metallic_texture >= 0
            || dst.roughness > 0.0f
            || dst.metallic > 0.0f;
        dst.shading_model = has_emissive && !has_principled
            ? imported_material_t::SHADING_EMISSIVE
            : (has_principled ? imported_material_t::SHADING_PRINCIPLED : imported_material_t::SHADING_LAMBERT);

        out.materials.push_back(dst);
    }

    for (size_t i = 0; i < obj.shapes.size(); ++i) {
        imported_shape_t dst;
        dst.name = obj.shapes[i].name;
        dst.shape = obj.shapes[i];
        if (!obj.shapes[i].mesh.materials.empty()) {
            dst.material_index = obj.shapes[i].mesh.materials[0];
        }
        out.shapes.push_back(dst);
    }

    return true;
}

static int import_fbx_texture(std::vector<imported_texture_t> &textures,
                              std::map<const ufbx_texture*, int> &texture_map,
                              const ufbx_texture *texture)
{
    if (!texture) return -1;

    std::map<const ufbx_texture*, int>::const_iterator found = texture_map.find(texture);
    if (found != texture_map.end()) return found->second;

    imported_texture_t dst;
    dst.name = to_std_string(texture->name);

    std::string filename = to_std_string(texture->filename);
    if (filename.empty()) filename = to_std_string(texture->relative_filename);
    if (filename.empty()) filename = to_std_string(texture->absolute_filename);

    if (texture->has_file && texture->file_index != UFBX_NO_INDEX) {
        dst.path = filename;
        dst.embedded_format = guess_format_from_path(dst.path);
    } else {
        dst.path = filename;
        dst.embedded_format = guess_format_from_path(dst.path);
    }

    if (texture->content.data && texture->content.size > 0) {
        dst.is_embedded = true;
        dst.embedded_bytes.assign((const unsigned char *)texture->content.data,
                                  (const unsigned char *)texture->content.data + texture->content.size);
    }

    textures.push_back(dst);
    const int index = (int)(textures.size() - 1);
    texture_map[texture] = index;
    return index;
}

static nimg::ColorRGBf vec3_to_color(ufbx_vec3 value)
{
    return nimg::ColorRGBf((float)value.x, (float)value.y, (float)value.z);
}

static void copy_color3(float dst[3], const nimg::ColorRGBf &src)
{
    dst[0] = src.r();
    dst[1] = src.g();
    dst[2] = src.b();
}

static int import_fbx_material(imported_asset_t &out,
                               std::map<const ufbx_texture*, int> &texture_map,
                               std::map<const ufbx_material*, int> &material_map,
                               const ufbx_material *material)
{
    if (!material) return -1;

    std::map<const ufbx_material*, int>::const_iterator found = material_map.find(material);
    if (found != material_map.end()) return found->second;

    imported_material_t dst;
    dst.name = to_std_string(material->name);

    const nimg::ColorRGBf base = material->pbr.base_color.has_value
        ? vec3_to_color(material->pbr.base_color.value_vec3)
        : vec3_to_color(material->fbx.diffuse_color.value_vec3);
    const nimg::ColorRGBf emissive = material->pbr.emission_color.has_value
        ? vec3_to_color(material->pbr.emission_color.value_vec3)
        : vec3_to_color(material->fbx.emission_color.value_vec3);
    const nimg::ColorRGBf transmission = material->pbr.transmission_color.has_value
        ? vec3_to_color(material->pbr.transmission_color.value_vec3)
        : nimg::ColorRGBf(0.0f, 0.0f, 0.0f);

    copy_color3(dst.base_color, base);
    copy_color3(dst.emissive, emissive);
    copy_color3(dst.transmission, transmission);

    dst.roughness = material->pbr.roughness.has_value ? (float)material->pbr.roughness.value_real : 0.0f;
    dst.metallic = material->pbr.metalness.has_value ? (float)material->pbr.metalness.value_real : 0.0f;
    dst.ior = material->pbr.specular_ior.has_value ? (float)material->pbr.specular_ior.value_real : 1.5f;
    dst.opacity = material->pbr.opacity.has_value ? (float)material->pbr.opacity.value_real : 1.0f;

    if (material->pbr.base_color.texture && material->pbr.base_color.texture_enabled) {
        dst.base_color_texture = import_fbx_texture(out.textures, texture_map, material->pbr.base_color.texture);
    } else if (material->fbx.diffuse_color.texture && material->fbx.diffuse_color.texture_enabled) {
        dst.base_color_texture = import_fbx_texture(out.textures, texture_map, material->fbx.diffuse_color.texture);
    }

    if (material->pbr.emission_color.texture && material->pbr.emission_color.texture_enabled) {
        dst.emissive_texture = import_fbx_texture(out.textures, texture_map, material->pbr.emission_color.texture);
    } else if (material->fbx.emission_color.texture && material->fbx.emission_color.texture_enabled) {
        dst.emissive_texture = import_fbx_texture(out.textures, texture_map, material->fbx.emission_color.texture);
    }

    if (material->pbr.normal_map.texture && material->pbr.normal_map.texture_enabled) {
        dst.normal_texture = import_fbx_texture(out.textures, texture_map, material->pbr.normal_map.texture);
    } else if (material->fbx.normal_map.texture && material->fbx.normal_map.texture_enabled) {
        dst.normal_texture = import_fbx_texture(out.textures, texture_map, material->fbx.normal_map.texture);
    } else if (material->fbx.bump.texture && material->fbx.bump.texture_enabled) {
        dst.normal_texture = import_fbx_texture(out.textures, texture_map, material->fbx.bump.texture);
    }

    if (material->pbr.roughness.texture && material->pbr.roughness.texture_enabled) {
        dst.roughness_texture = import_fbx_texture(out.textures, texture_map, material->pbr.roughness.texture);
    }
    if (material->pbr.metalness.texture && material->pbr.metalness.texture_enabled) {
        dst.metallic_texture = import_fbx_texture(out.textures, texture_map, material->pbr.metalness.texture);
    }
    if (material->pbr.transmission_color.texture && material->pbr.transmission_color.texture_enabled) {
        dst.transmission_texture = import_fbx_texture(out.textures, texture_map, material->pbr.transmission_color.texture);
    }

    const bool has_emissive = dst.emissive_texture >= 0
        || dst.emissive[0] > 0.0f
        || dst.emissive[1] > 0.0f
        || dst.emissive[2] > 0.0f;
    const bool has_principled = material->features.pbr.enabled
        || material->features.metalness.enabled
        || dst.roughness_texture >= 0
        || dst.metallic_texture >= 0
        || dst.roughness > 0.0f
        || dst.metallic > 0.0f
        || material->shader_type == UFBX_SHADER_GLTF_MATERIAL
        || material->shader_type == UFBX_SHADER_3DS_MAX_PBR_METAL_ROUGH
        || material->shader_type == UFBX_SHADER_ARNOLD_STANDARD_SURFACE
        || material->shader_type == UFBX_SHADER_OSL_STANDARD_SURFACE
        || material->shader_type == UFBX_SHADER_BLENDER_PHONG;

    dst.shading_model = has_emissive && !has_principled
        ? imported_material_t::SHADING_EMISSIVE
        : (has_principled ? imported_material_t::SHADING_PRINCIPLED : imported_material_t::SHADING_LAMBERT);

    out.materials.push_back(dst);
    const int index = (int)(out.materials.size() - 1);
    material_map[material] = index;
    return index;
}

static bool import_asset_fbx(const char *path, imported_asset_t &out, std::string &err)
{
    ufbx_load_opts opts = { };
    opts.generate_missing_normals = true;
    opts.use_blender_pbr_material = true;

    ufbx_error load_error;
    ufbx_scene *scene = ufbx_load_file(path, &opts, &load_error);
    if (!scene) {
        char buffer[512];
        ufbx_format_error(buffer, sizeof(buffer), &load_error);
        err = buffer;
        return false;
    }

    out.source_path = path ? std::string(path) : std::string();
    std::string base, file;
    ncf::util::path_comp(out.source_path, base, file);
    out.base_dir = base;
    out.attributes = nmesh::attrib_t();
    out.textures.clear();
    out.materials.clear();
    out.shapes.clear();

    std::map<const ufbx_texture*, int> texture_map;
    std::map<const ufbx_material*, int> material_map;

    for (size_t node_ix = 0; node_ix < scene->nodes.count; ++node_ix) {
        const ufbx_node *node = scene->nodes.data[node_ix];
        if (!node || !node->mesh) continue;

        const ufbx_mesh *mesh = node->mesh;
        const ufbx_matrix normal_to_world = ufbx_matrix_for_normals(&node->geometry_to_world);

        struct shape_builder_t {
            imported_shape_t shape;
            bool used;
            shape_builder_t() : shape(), used(false) {}
        };

        std::map<int, shape_builder_t> builders;
        std::vector<uint32_t> tri_indices(mesh->max_face_triangles * 3);

        for (size_t face_ix = 0; face_ix < mesh->faces.count; ++face_ix) {
            const ufbx_face face = mesh->faces.data[face_ix];
            if (face.num_indices < 3) continue;

            uint32_t face_material = 0;
            if (mesh->face_material.count > face_ix) {
                face_material = mesh->face_material.data[face_ix];
            }

            int imported_material_index = -1;
            if (face_material != UFBX_NO_INDEX && face_material < node->materials.count) {
                imported_material_index = import_fbx_material(out, texture_map, material_map, node->materials.data[face_material]);
            } else if (face_material != UFBX_NO_INDEX && face_material < mesh->materials.count) {
                imported_material_index = import_fbx_material(out, texture_map, material_map, mesh->materials.data[face_material]);
            }

            shape_builder_t &builder = builders[imported_material_index];
            if (!builder.used) {
                builder.used = true;
                builder.shape.material_index = imported_material_index;
                builder.shape.name = to_std_string(node->name);
                if (imported_material_index >= 0) {
                    builder.shape.name.append("_");
                    builder.shape.name.append(out.materials[(size_t)imported_material_index].name);
                }
            }

            const uint32_t num_triangles = ufbx_triangulate_face(tri_indices.data(), tri_indices.size(), mesh, face);
            for (uint32_t tri = 0; tri < num_triangles; ++tri) {
                for (uint32_t corner = 0; corner < 3; ++corner) {
                    const size_t index = tri_indices[tri * 3 + corner];

                    const ufbx_vec3 pos = ufbx_transform_position(&node->geometry_to_world,
                        ufbx_get_vertex_vec3(&mesh->vertex_position, index));
                    out.attributes.v.push_back((float)pos.x);
                    out.attributes.v.push_back((float)pos.y);
                    out.attributes.v.push_back((float)pos.z);

                    nmesh::index_t dst_index;
                    dst_index.v = (int)(out.attributes.v.size() / 3u) - 1;
                    dst_index.n = -1;
                    dst_index.uv = -1;

                    if (mesh->vertex_normal.exists) {
                        ufbx_vec3 nrm = ufbx_transform_direction(&normal_to_world, ufbx_get_vertex_vec3(&mesh->vertex_normal, index));
                        nrm = ufbx_vec3_normalize(nrm);
                        out.attributes.n.push_back((float)nrm.x);
                        out.attributes.n.push_back((float)nrm.y);
                        out.attributes.n.push_back((float)nrm.z);
                        dst_index.n = (int)(out.attributes.n.size() / 3u) - 1;
                    }

                    if (mesh->vertex_uv.exists) {
                        const ufbx_vec2 uv = ufbx_get_vertex_vec2(&mesh->vertex_uv, index);
                        out.attributes.uv.push_back((float)uv.x);
                        out.attributes.uv.push_back((float)uv.y);
                        dst_index.uv = (int)(out.attributes.uv.size() / 2u) - 1;
                    }

                    builder.shape.shape.mesh.indices.push_back(dst_index);
                }
                builder.shape.shape.mesh.materials.push_back(imported_material_index);
            }
        }

        for (std::map<int, shape_builder_t>::iterator it = builders.begin(); it != builders.end(); ++it) {
            if (!it->second.used || it->second.shape.shape.mesh.indices.empty()) continue;
            out.shapes.push_back(it->second.shape);
        }
    }

    ufbx_free_scene(scene);

    if (out.shapes.empty()) {
        err = "FBX asset contains no mesh geometry";
        return false;
    }

    return true;
}

static int append_embedded_texture(std::vector<imported_texture_t> &textures,
                                   const std::string &name,
                                   const std::string &path,
                                   const std::vector<unsigned char> &bytes,
                                   const std::string &format)
{
    imported_texture_t tex;
    tex.name = name;
    tex.path = path;
    tex.is_embedded = !bytes.empty();
    tex.embedded_bytes = bytes;
    tex.embedded_format = format;
    textures.push_back(tex);
    return (int)(textures.size() - 1);
}

static int import_gltf_texture(imported_asset_t &out,
                               std::map<const cgltf_texture*, int> &texture_map,
                               const cgltf_texture *texture)
{
    if (!texture) return -1;

    std::map<const cgltf_texture*, int>::const_iterator found = texture_map.find(texture);
    if (found != texture_map.end()) return found->second;

    const cgltf_image *image = texture->image;
    if (!image) return -1;

    const std::string texture_name = sanitize_import_name(texture->name ? texture->name : "", "gltf_texture");
    const std::string image_name = sanitize_import_name(image->name ? image->name : "", texture_name.c_str());
    const std::string uri = image->uri ? std::string(image->uri) : std::string();
    const std::string mime = image->mime_type ? std::string(image->mime_type) : std::string();

    if (!uri.empty() && uri.find("data:") == 0) {
        const size_t comma = uri.find(',');
        if (comma != std::string::npos) {
            void *decoded = 0;
            if (cgltf_load_buffer_base64(0, (cgltf_size)-1, uri.c_str() + comma + 1, &decoded) == cgltf_result_success && decoded) {
                // cgltf does not expose decoded byte length, so infer from decoded image decode later is not safe.
                // Use image_memory() consumer semantics by storing a null-terminated blob is not valid.
                // Fall back to file path if we cannot determine size.
                // For data URIs we instead keep the original URI pathless and let file fallback fail cleanly.
                free(decoded);
            }
        }
    }

    if (image->buffer_view && image->buffer_view->buffer && image->buffer_view->buffer->data) {
        const unsigned char *base = (const unsigned char *)(image->buffer_view->data
            ? image->buffer_view->data
            : image->buffer_view->buffer->data);
        const size_t offset = image->buffer_view->data ? 0u : (size_t)image->buffer_view->offset;
        const size_t size = (size_t)image->buffer_view->size;
        std::vector<unsigned char> bytes;
        bytes.assign(base + offset, base + offset + size);
        int index = append_embedded_texture(out.textures, image_name, uri, bytes, guess_format_from_path(uri.empty() ? mime : uri));
        texture_map[texture] = index;
        return index;
    }

    if (!uri.empty()) {
        imported_texture_t tex;
        tex.name = image_name;
        tex.path = uri;
        tex.embedded_format = guess_format_from_path(uri.empty() ? mime : uri);
        out.textures.push_back(tex);
        const int index = (int)(out.textures.size() - 1);
        texture_map[texture] = index;
        return index;
    }

    return -1;
}

static int import_gltf_texture_view(imported_asset_t &out,
                                    std::map<const cgltf_texture*, int> &texture_map,
                                    const cgltf_texture_view &view,
                                    const std::string &source_path)
{
    if (!view.texture) return -1;
    (void)source_path;
    return import_gltf_texture(out, texture_map, view.texture);
}

static int import_gltf_material(imported_asset_t &out,
                                std::map<const cgltf_texture*, int> &texture_map,
                                std::map<const cgltf_material*, int> &material_map,
                                const cgltf_material *material,
                                const std::string &source_path)
{
    if (!material) return -1;

    std::map<const cgltf_material*, int>::const_iterator found = material_map.find(material);
    if (found != material_map.end()) return found->second;

    imported_material_t dst;
    dst.name = sanitize_import_name(material->name ? material->name : "", "gltf_material");
    dst.shading_model = imported_material_t::SHADING_PRINCIPLED;

    const cgltf_pbr_metallic_roughness &pbr = material->pbr_metallic_roughness;
    dst.base_color[0] = pbr.base_color_factor[0];
    dst.base_color[1] = pbr.base_color_factor[1];
    dst.base_color[2] = pbr.base_color_factor[2];
    dst.roughness = pbr.roughness_factor;
    dst.metallic = pbr.metallic_factor;
    dst.ior = material->has_ior ? material->ior.ior : 1.5f;
    dst.opacity = pbr.base_color_factor[3];
    dst.emissive[0] = material->emissive_factor[0];
    dst.emissive[1] = material->emissive_factor[1];
    dst.emissive[2] = material->emissive_factor[2];
    if (material->has_transmission) {
        dst.transmission[0] = material->transmission.transmission_factor;
        dst.transmission[1] = material->transmission.transmission_factor;
        dst.transmission[2] = material->transmission.transmission_factor;
    }

    dst.base_color_texture = import_gltf_texture_view(out, texture_map, pbr.base_color_texture, source_path);
    dst.roughness_texture = import_gltf_texture_view(out, texture_map, pbr.metallic_roughness_texture, source_path);
    dst.metallic_texture = dst.roughness_texture;
    dst.normal_texture = import_gltf_texture_view(out, texture_map, material->normal_texture, source_path);
    dst.emissive_texture = import_gltf_texture_view(out, texture_map, material->emissive_texture, source_path);
    if (material->has_transmission) {
        dst.transmission_texture = import_gltf_texture_view(out, texture_map, material->transmission.transmission_texture, source_path);
    }

    const bool has_emissive = dst.emissive_texture >= 0
        || dst.emissive[0] > 0.0f
        || dst.emissive[1] > 0.0f
        || dst.emissive[2] > 0.0f;
    if (has_emissive
        && dst.base_color_texture < 0
        && dst.base_color[0] <= 0.0f
        && dst.base_color[1] <= 0.0f
        && dst.base_color[2] <= 0.0f) {
        dst.shading_model = imported_material_t::SHADING_EMISSIVE;
    }

    out.materials.push_back(dst);
    const int index = (int)(out.materials.size() - 1);
    material_map[material] = index;
    return index;
}

static bool read_accessor_vec3(const cgltf_accessor *accessor, cgltf_size index, float out[3])
{
    if (!accessor) return false;
    cgltf_float tmp[4] = { 0, 0, 0, 0 };
    if (!cgltf_accessor_read_float(accessor, index, tmp, 3)) return false;
    out[0] = tmp[0];
    out[1] = tmp[1];
    out[2] = tmp[2];
    return true;
}

static bool read_accessor_vec2(const cgltf_accessor *accessor, cgltf_size index, float out[2])
{
    if (!accessor) return false;
    cgltf_float tmp[4] = { 0, 0, 0, 0 };
    if (!cgltf_accessor_read_float(accessor, index, tmp, 2)) return false;
    out[0] = tmp[0];
    out[1] = tmp[1];
    return true;
}

static cgltf_accessor *find_gltf_attribute(const cgltf_primitive &primitive, cgltf_attribute_type type, int index)
{
    for (cgltf_size i = 0; i < primitive.attributes_count; ++i) {
        const cgltf_attribute &attr = primitive.attributes[i];
        if (attr.type == type && attr.index == index) return attr.data;
    }
    return 0;
}

static bool import_asset_gltf(const char *path, imported_asset_t &out, std::string &err)
{
    cgltf_options options = { };
    cgltf_data *data = 0;
    if (cgltf_parse_file(&options, path, &data) != cgltf_result_success || !data) {
        err = "failed to parse glTF";
        return false;
    }
    if (cgltf_load_buffers(&options, data, path) != cgltf_result_success) {
        cgltf_free(data);
        err = "failed to load glTF buffers";
        return false;
    }
    if (cgltf_validate(data) != cgltf_result_success) {
        cgltf_free(data);
        err = "invalid glTF";
        return false;
    }

    out.source_path = path ? std::string(path) : std::string();
    std::string base, file;
    ncf::util::path_comp(out.source_path, base, file);
    out.base_dir = base;
    out.attributes = nmesh::attrib_t();
    out.textures.clear();
    out.materials.clear();
    out.shapes.clear();

    std::map<const cgltf_texture*, int> texture_map;
    std::map<const cgltf_material*, int> material_map;

    for (cgltf_size node_ix = 0; node_ix < data->nodes_count; ++node_ix) {
        const cgltf_node &node = data->nodes[node_ix];
        if (!node.mesh) continue;

        cgltf_float mat[16];
        cgltf_node_transform_world(&node, mat);

        for (cgltf_size prim_ix = 0; prim_ix < node.mesh->primitives_count; ++prim_ix) {
            const cgltf_primitive &primitive = node.mesh->primitives[prim_ix];
            if (primitive.type != cgltf_primitive_type_triangles) continue;

            cgltf_accessor *positions = find_gltf_attribute(primitive, cgltf_attribute_type_position, 0);
            if (!positions) continue;
            cgltf_accessor *normals = find_gltf_attribute(primitive, cgltf_attribute_type_normal, 0);
            cgltf_accessor *uvs = find_gltf_attribute(primitive, cgltf_attribute_type_texcoord, 0);

            imported_shape_t shape;
            shape.name = sanitize_import_name(node.name ? node.name : "", node.mesh->name ? node.mesh->name : "gltf_mesh");
            if (node.mesh->primitives_count > 1) {
                shape.name.append("_");
                shape.name.append(std::to_string((int)prim_ix));
            }
            shape.material_index = import_gltf_material(out, texture_map, material_map, primitive.material, out.source_path);

            const cgltf_size index_count = primitive.indices ? primitive.indices->count : positions->count;
            for (cgltf_size i = 0; i < index_count; ++i) {
                const cgltf_size vertex_index = primitive.indices ? cgltf_accessor_read_index(primitive.indices, i) : i;

                float pos[3] = { 0, 0, 0 };
                if (!read_accessor_vec3(positions, vertex_index, pos)) continue;

                const float x = pos[0], y = pos[1], z = pos[2];
                const float tx = mat[0] * x + mat[4] * y + mat[8]  * z + mat[12];
                const float ty = mat[1] * x + mat[5] * y + mat[9]  * z + mat[13];
                const float tz = mat[2] * x + mat[6] * y + mat[10] * z + mat[14];
                out.attributes.v.push_back(tx);
                out.attributes.v.push_back(ty);
                out.attributes.v.push_back(tz);

                nmesh::index_t dst_index;
                dst_index.v = (int)(out.attributes.v.size() / 3u) - 1;
                dst_index.n = -1;
                dst_index.uv = -1;

                if (normals) {
                    float nrm[3] = { 0, 0, 0 };
                    if (read_accessor_vec3(normals, vertex_index, nrm)) {
                        const float nx = mat[0] * nrm[0] + mat[4] * nrm[1] + mat[8]  * nrm[2];
                        const float ny = mat[1] * nrm[0] + mat[5] * nrm[1] + mat[9]  * nrm[2];
                        const float nz = mat[2] * nrm[0] + mat[6] * nrm[1] + mat[10] * nrm[2];
                        const float len = std::sqrt(nx * nx + ny * ny + nz * nz);
                        const float inv = len > 1e-8f ? 1.0f / len : 1.0f;
                        out.attributes.n.push_back(nx * inv);
                        out.attributes.n.push_back(ny * inv);
                        out.attributes.n.push_back(nz * inv);
                        dst_index.n = (int)(out.attributes.n.size() / 3u) - 1;
                    }
                }

                if (uvs) {
                    float uv[2] = { 0, 0 };
                    if (read_accessor_vec2(uvs, vertex_index, uv)) {
                        out.attributes.uv.push_back(uv[0]);
                        out.attributes.uv.push_back(uv[1]);
                        dst_index.uv = (int)(out.attributes.uv.size() / 2u) - 1;
                    }
                }

                shape.shape.mesh.indices.push_back(dst_index);
            }

            for (cgltf_size face = 0; face < index_count / 3u; ++face) {
                shape.shape.mesh.materials.push_back(shape.material_index);
            }

            if (!shape.shape.mesh.indices.empty()) out.shapes.push_back(shape);
        }
    }

    cgltf_free(data);

    if (out.shapes.empty()) {
        err = "glTF asset contains no triangle mesh geometry";
        return false;
    }
    return true;
}

} // namespace

bool import_asset_file(const char *path, imported_asset_t &out, std::string &err)
{
    if (!path || !*path) {
        err = "empty import path";
        return false;
    }

    const std::string ext = lowercase_extension(path);
    if (ext == "obj") return import_asset_obj(path, out, err);
    if (ext == "fbx") return import_asset_fbx(path, out, err);
    if (ext == "gltf" || ext == "glb") return import_asset_gltf(path, out, err);

    err = "unsupported asset format";
    return false;
}

xtcore::asset::ISurface *build_surface_from_imported_asset(const imported_asset_t &asset, std::string &err)
{
    if (asset.shapes.empty()) {
        err = "asset contains no shapes";
        return 0;
    }

    xtcore::surface::Mesh *mesh = new (std::nothrow) xtcore::surface::Mesh();
    if (!mesh) {
        err = "failed to allocate mesh surface";
        return 0;
    }

    if (asset.shapes.size() == 1) {
        nmesh::shape_t shape = asset.shapes[0].shape;
        nmesh::attrib_t attributes = asset.attributes;
        mesh->build_octree(shape, attributes);
        return mesh;
    }

    nmesh::object_t object;
    object.attributes = asset.attributes;
    for (size_t i = 0; i < asset.shapes.size(); ++i) {
        object.shapes.push_back(asset.shapes[i].shape);
    }
    mesh->build_octree(object);
    return mesh;
}

int create_objects_from_imported_asset(xtcore::Scene *scene,
                                       const imported_asset_t &asset,
                                       const char *prefix,
                                       const xtcore::asset::medium::IMedium *medium_proto,
                                       std::string &err)
{
    if (!scene) {
        err = "scene is null";
        return 1;
    }
    if (asset.shapes.empty()) {
        err = "asset contains no shapes";
        return 1;
    }

    std::vector<HASH_UINT64> matids;
    matids.reserve(asset.materials.size());

    for (size_t m = 0; m < asset.materials.size(); ++m) {
        std::string name = prefix ? std::string(prefix) : std::string();
        name.append(asset.materials[m].name);
        HASH_UINT64 id = xtcore::pool::str::add(name.c_str());

        bool loaded = false;
        for (size_t i = 0; i < matids.size(); ++i) {
            if (matids[i] == id) {
                loaded = true;
                break;
            }
        }
        if (loaded) continue;

        xtcore::asset::IMaterial *mat = build_runtime_material(asset, asset.materials[m]);
        if (!mat) {
            err = "failed to allocate imported material";
            return 1;
        }

        matids.push_back(id);
        scene->m_materials[id] = mat;
    }

    HASH_UINT64 fallback_mat_id = HASH_ID_INVALID;
    for (size_t i = 0; i < asset.shapes.size(); ++i) {
        const imported_shape_t &shape = asset.shapes[i];

        std::string name = std::to_string((int)i + 1);
        name.append(prefix ? prefix : "");
        name.append(shape.name);
        HASH_UINT64 id = xtcore::pool::str::add(name.c_str());

        xtcore::surface::Mesh *surf = new (std::nothrow) xtcore::surface::Mesh();
        if (!surf) {
            err = "failed to allocate imported mesh";
            return 1;
        }

        nmesh::shape_t shape_copy = shape.shape;
        nmesh::attrib_t attr_copy = asset.attributes;
        surf->build_octree(shape_copy, attr_copy);
        scene->m_surface[id] = surf;

        xtcore::asset::Object *obj = new (std::nothrow) xtcore::asset::Object();
        if (!obj) {
            err = "failed to allocate imported object";
            return 1;
        }
        obj->surface = id;

        if (shape.material_index < 0 || (size_t)shape.material_index >= matids.size()) {
            if (fallback_mat_id == HASH_ID_INVALID) {
                fallback_mat_id = ensure_fallback_material(scene, asset, prefix);
                if (fallback_mat_id == HASH_ID_INVALID) {
                    err = "failed to allocate fallback material";
                    return 1;
                }
            }
            obj->material = fallback_mat_id;
        } else {
            obj->material = matids[(size_t)shape.material_index];
        }

        scene->m_objects[id] = obj;
        if (medium_proto) {
            scene->set_object_medium(id, medium_proto->clone());
            if (!scene->has_object_medium(id)) {
                err = "failed to bind medium to imported object";
                return 1;
            }
        }
    }

    scene->mark_spatial_index_dirty();
    return 0;
}

} /* namespace xtcore */
