#ifndef NMESH_MESH_HPP_INCLUDED
#define NMESH_MESH_HPP_INCLUDED

#include <vector>

namespace NMesh {

/*
typedef struct {
  std::string name;

  float ambient[3];
  float diffuse[3];
  float specular[3];
  float transmittance[3];
  float emission[3];
  float shininess;
  float ior;       // index of refraction
  float dissolve;  // 1 == opaque; 0 == fully transparent
  // illumination model (see http://www.fileformat.info/format/material/)
  int illum;

  int dummy;  // Suppress padding warning.

  std::string ambient_texname;             // map_Ka
  std::string diffuse_texname;             // map_Kd
  std::string specular_texname;            // map_Ks
  std::string specular_highlight_texname;  // map_Ns
  std::string bump_texname;                // map_bump, bump
  std::string displacement_texname;        // disp
  std::string alpha_texname;               // map_d

  // PBR extension
  // http://exocortex.com/blog/extending_wavefront_mtl_to_support_pbr
  float roughness;                // [0, 1] default 0
  float metallic;                 // [0, 1] default 0
  float sheen;                    // [0, 1] default 0
  float clearcoat_thickness;      // [0, 1] default 0
  float clearcoat_roughness;      // [0, 1] default 0
  float anisotropy;               // aniso. [0, 1] default 0
  float anisotropy_rotation;      // anisor. [0, 1] default 0
  std::string roughness_texname;  // map_Pr
  std::string metallic_texname;   // map_Pm
  std::string sheen_texname;      // map_Ps
  std::string emissive_texname;   // map_Ke
  std::string normal_texname;     // norm. For normal mapping.

  std::map<std::string, std::string> unknown_parameter;
} material_t;
*/

typedef struct {
    int v;	// vertex
    int n;  // normal
    int uv; // texture coordinates
} index_t;

typedef struct {
    std::vector<float> v;	// vertex
    std::vector<float> n;   // normal
    std::vector<float> uv;  // texture coordinates
} attrib_t;

typedef struct {
  std::vector<index_t> indices;
  std::vector<int>     materials;
} mesh_t;

typedef struct {
    std::string name;
    mesh_t mesh;
} shape_t;

typedef struct {
	std::vector<shape_t> shapes;
    attrib_t             attributes;
} object_t;

} /* namespace NMesh */

#endif /* NMESH_MESH_HPP_INCLUDED */
