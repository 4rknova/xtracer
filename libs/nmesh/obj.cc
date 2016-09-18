#include <cstdio>
#include <string>
#include <vector>
#include "tiny_obj_loader.h"
#include "obj.h"

namespace NMesh {
	namespace IO {
		namespace Import {

int obj(const char* filename, object_t &obj)
{
	if(!filename) return 1;

	tinyobj::attrib_t attribs;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err;
	bool loaded = tinyobj::LoadObj(&attribs, &shapes, &materials, &err, filename);

	if (!loaded) return 1;

	for (size_t s = 0; s < shapes.size(); ++s) {
        shape_t shape;

        shape.name = shapes[s].name;

        // load indices
		for (size_t i = 0; i < shapes[s].mesh.indices.size(); ++i) {
            index_t idx;
            idx.v  = shapes[s].mesh.indices[i].vertex_index;
            idx.n  = shapes[s].mesh.indices[i].normal_index;
            idx.uv = shapes[s].mesh.indices[i].texcoord_index;
            shape.mesh.indices.push_back(idx);
   	  	}

        // load material ids
        for (size_t i = 0; i < shapes[s].mesh.material_ids.size(); ++i) {
            shape.mesh.materials.push_back(shapes[s].mesh.material_ids[i]);
        }

        obj.shapes.push_back(shape);
	}

    // Load attributes
    {
        for (size_t i = 0; i < attribs.vertices.size(); ++i) {
            obj.attributes.v.push_back(attribs.vertices[i]);
        }

        for (size_t i = 0; i < attribs.normals.size(); ++i) {
            obj.attributes.n.push_back(attribs.normals[i]);
        }

        for (size_t i = 0; i < attribs.texcoords.size(); ++i) {
            obj.attributes.uv.push_back(attribs.texcoords[i]);
        }
    }

    // Load materials
    for (size_t i = 0; i < materials.size(); ++i) {
        material_t mat;

        tinyobj::material_t *m = &(materials[i]);

        mat.name = m->name;

        mat.ambient[0] = m->ambient[0];
        mat.ambient[1] = m->ambient[1];
        mat.ambient[2] = m->ambient[2];

        mat.diffuse[0] = m->diffuse[0];
        mat.diffuse[1] = m->diffuse[1];
        mat.diffuse[2] = m->diffuse[2];

        mat.specular[0] = m->specular[0];
        mat.specular[1] = m->specular[1];
        mat.specular[2] = m->specular[2];

        mat.transmittance[0] = m->transmittance[0];
        mat.transmittance[1] = m->transmittance[1];
        mat.transmittance[2] = m->transmittance[2];

        mat.emission[0] = m->emission[0];
        mat.emission[1] = m->emission[1];
        mat.emission[2] = m->emission[2];

        mat.shininess     = m->shininess;
        mat.ior           = m->ior;
        mat.dissolve      = m->dissolve;
        mat.illum         = m->illum;
        mat.dummy         = m->dummy;

        mat.texture_ambient            = m->ambient_texname;
        mat.texture_diffuse            = m->diffuse_texname;
        mat.texture_specular           = m->specular_texname;
        mat.texture_specular_highlight = m->specular_highlight_texname;
        mat.texture_bump               = m->bump_texname;
        mat.texture_displacement       = m->displacement_texname;
        mat.texture_alpha              = m->alpha_texname;

        mat.roughness           = m->roughness;
        mat.metallic            = m->metallic;
        mat.sheen               = m->sheen;
        mat.clearcoat_thickness = m->clearcoat_thickness;
        mat.clearcoat_roughness = m->clearcoat_roughness;
        mat.anisotropy          = m->anisotropy;
        mat.anisotropy_rotation = m->anisotropy_rotation;

        mat.texture_roughness   = m->roughness_texname;
        mat.texture_metallic    = m->metallic_texname;
        mat.texture_sheen       = m->sheen_texname;
        mat.texture_emissive    = m->emissive_texname;
        mat.texture_normal      = m->normal_texname;

        std::map<std::string, std::string>::iterator it = m->unknown_parameter.begin();
        std::map<std::string, std::string>::iterator et = m->unknown_parameter.end();

        for (; it != et; ++it) {
             mat.parameters[it->first] = it->second;
        }

        obj.materials.push_back(mat);
    }

	return 0;
}

		} /* namespace Import */
	} /* namespace IO */
} /* namespace NMesh */
