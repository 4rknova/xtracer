#ifndef XTCORE_IMPORT_ASSET_H_INCLUDED
#define XTCORE_IMPORT_ASSET_H_INCLUDED

#include <string>
#include <vector>

#include <nmesh/structs.h>

#include "scene.h"

namespace xtcore {

struct imported_texture_t
{
    std::string name;
    std::string path;
    std::vector<unsigned char> embedded_bytes;
    std::string embedded_format;
    bool is_embedded;

    imported_texture_t()
        : name()
        , path()
        , embedded_bytes()
        , embedded_format()
        , is_embedded(false)
    {}
};

struct imported_material_t
{
    enum shading_model_t {
        SHADING_UNKNOWN = 0,
        SHADING_LAMBERT,
        SHADING_PRINCIPLED,
        SHADING_EMISSIVE,
        SHADING_DIELECTRIC
    };

    std::string name;
    shading_model_t shading_model;

    float base_color[3];
    float emissive[3];
    float transmission[3];
    float roughness;
    float metallic;
    float ior;
    float opacity;

    int base_color_texture;
    int emissive_texture;
    int normal_texture;
    int roughness_texture;
    int metallic_texture;
    int transmission_texture;

    imported_material_t()
        : name()
        , shading_model(SHADING_UNKNOWN)
        , roughness(0.0f)
        , metallic(0.0f)
        , ior(1.0f)
        , opacity(1.0f)
        , base_color_texture(-1)
        , emissive_texture(-1)
        , normal_texture(-1)
        , roughness_texture(-1)
        , metallic_texture(-1)
        , transmission_texture(-1)
    {
        base_color[0] = 0.0f; base_color[1] = 0.0f; base_color[2] = 0.0f;
        emissive[0] = 0.0f; emissive[1] = 0.0f; emissive[2] = 0.0f;
        transmission[0] = 0.0f; transmission[1] = 0.0f; transmission[2] = 0.0f;
    }
};

struct imported_shape_t
{
    std::string name;
    nmesh::shape_t shape;
    int material_index;

    imported_shape_t()
        : name()
        , shape()
        , material_index(-1)
    {}
};

struct imported_asset_t
{
    std::string source_path;
    std::string base_dir;
    nmesh::attrib_t attributes;
    std::vector<imported_texture_t> textures;
    std::vector<imported_material_t> materials;
    std::vector<imported_shape_t> shapes;
};

bool import_asset_file(const char *path, imported_asset_t &out, std::string &err);
xtcore::asset::ISurface *build_surface_from_imported_asset(const imported_asset_t &asset, std::string &err);
int create_objects_from_imported_asset(xtcore::Scene *scene,
                                       const imported_asset_t &asset,
                                       const char *prefix,
                                       const xtcore::asset::medium::IMedium *medium_proto,
                                       std::string &err);

} /* namespace xtcore */

#endif /* XTCORE_IMPORT_ASSET_H_INCLUDED */
