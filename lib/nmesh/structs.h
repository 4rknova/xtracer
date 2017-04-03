#ifndef NMESH_STRUCTS_H_INCLUDED
#define NMESH_STRUCTS_H_INCLUDED

#include <string>
#include <vector>
#include <map>

namespace nmesh {

typedef struct {
  std::string name;

  float ambient[3];
  float diffuse[3];
  float specular[3];
  float transmittance[3];
  float emission[3];
  float shininess;
  float ior;               // index of refraction
  float dissolve;          // 1 == opaque; 0 == fully transparent
  int   illum;             // illumination model (see http://www.fileformat.info/format/material/)
  int   dummy;             // Suppress padding warning.

  std::string texture_ambient;             // map_Ka
  std::string texture_diffuse;             // map_Kd
  std::string texture_specular;            // map_Ks
  std::string texture_specular_highlight;  // map_Ns
  std::string texture_bump;                // map_bump, bump
  std::string texture_displacement;        // disp
  std::string texture_alpha;               // map_d

  // PBR extension
  // http://exocortex.com/blog/extending_wavefront_mtl_to_support_pbr
  float roughness;                // [0, 1] default 0
  float metallic;                 // [0, 1] default 0
  float sheen;                    // [0, 1] default 0
  float clearcoat_thickness;      // [0, 1] default 0
  float clearcoat_roughness;      // [0, 1] default 0
  float anisotropy;               // aniso. [0, 1] default 0
  float anisotropy_rotation;      // anisor. [0, 1] default 0
  std::string texture_roughness;  // map_Pr
  std::string texture_metallic;   // map_Pm
  std::string texture_sheen;      // map_Ps
  std::string texture_emissive;   // map_Ke
  std::string texture_normal;     // norm. For normal mapping.

  std::map<std::string, std::string> parameters;
} material_t;

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
	std::vector<shape_t>    shapes;
    attrib_t                attributes;
    std::vector<material_t> materials;
} object_t;

} /* namespace nmesh */

#endif /* NMESH_STRUCTS_H_INCLUDED */
