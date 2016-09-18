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

	return 0;
}

		} /* namespace Import */
	} /* namespace IO */
} /* namespace NMesh */
