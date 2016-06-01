#include <cstdio>
#include <string>
#include <vector>

#include <nmath/precision.h>
#include <nmath/vector.h>

#include "buffer.hpp"
#include "vertex.h"
#include "obj.hpp"

namespace NMesh {
		namespace IO {
			namespace Import {
/*
	NOTES:
	This is implemented as a single pass parser.
	Due to the way that obj stores the vertex data
	some vertices will be duplicated.
*/

/*
	FORMAT: OBJ (Restrictions apply. See header file for details.)
	TYPE: ASCII

	HEADER:
	No header data.
*/

int obj(const char* filename, NMesh::Mesh &mesh)
{
	if(!filename)
		return 1;

	FILE *fp = fopen(filename, "r");

	if (!fp)
		return 3;

	// Temporary data buffers
	std::vector<NMath::vec2_t> uv_coord;
	std::vector<NMath::vec3_t> position;
	std::vector<NMath::vec3_t> normal;

	std::vector<vertex_t> vertices;
	std::vector<index_t> indices;

	unsigned int idx = 0;

	bool has_uvs = false;
	bool has_normals = false;

	// Import the data
	char line[512];
	// Zero out the buffer.
	memset(line, '\0', sizeof(line) * sizeof(char));

	while(fgets(line,sizeof(line),fp)) {
		if (!line[0] || line[0] == '#')
			continue;

		bool syntax_error = false; // Flag to check for syntax errors

		vertex_t vert;
		float x, y, z, u, v;
		int i1, i2, i3, i4, i5, i6, i7, i8, i9;

		switch (line[0]) {
			case 'v':
			{
				if (line[1]) {
					switch (line[1]) {
						case ' ':
						{
							if(sscanf(line, "v %f %f %f",
								&x, &y, &z) != 3) {
								syntax_error = true;
								break;
							}

							NMath::vec3_t dv = {x, y, z};
							position.push_back(dv);
							break;
						}
						case 't':
						{
							if(sscanf(line, "vt %f %f",
								&u, &v) != 2) {
								syntax_error = true;
								break;
							}

							NMath::vec2_t dv = {u, 1.0f - v};
							uv_coord.push_back(dv);
							break;
						}

						case 'n':
						{
							if(sscanf(line, "vn %f %f %f",
								&x, &y, &z) != 3) {
								syntax_error = true;
								break;
							}

							NMath::vec3_t dv = {x, y, z};
							normal.push_back(dv);
							break;
						}

					}
				}
				break;
			}
			case 'f':
			{
				if(sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d",
						&i1, &i2, &i3, &i4, &i5, &i6, &i7, &i8, &i9 ) == 9)	{
					has_uvs = true;
					has_normals = true;
				}
				else if (sscanf(line, "f %d/%d %d/%d %d/%d",
						&i1, &i2, &i4, &i5, &i7, &i8 ) == 6) {
						i3 = i6 = i9 = 0;
					has_uvs = true;
				}
				else if (sscanf(line, "f %d//%d %d//%d %d//%d",
						&i1, &i3, &i4, &i6, &i7, &i9 ) == 6) {
						i2 = i5 = i8 = 0;
					has_normals = true;
				}
				else if (sscanf(line, "f %d %d %d",
						&i1, &i4, &i7 ) == 3) {
						i2 = i3 = i5 = i6 = i8 = i9 = 0;
				}
				else {
					syntax_error = true;
					break;
				}

				// This is an important security check for malformed files.
				if(	   (unsigned int)i1 > position.size()
					|| (unsigned int)i2 > uv_coord.size()
					|| (unsigned int)i3 > normal.size()
					|| (unsigned int)i4 > position.size()
					|| (unsigned int)i5 > uv_coord.size()
					|| (unsigned int)i6 > normal.size()
					|| (unsigned int)i7 > position.size()
					|| (unsigned int)i8 > uv_coord.size()
					|| (unsigned int)i9 > normal.size()
					) {
					syntax_error = true;

					break;
				}

				if (position.empty()) {
					syntax_error = true;
					break;
				}

				// 1st vertex
				vert.px = (NMath::scalar_t)position[i1-1].x;
				vert.py = (NMath::scalar_t)position[i1-1].y;
				vert.pz = (NMath::scalar_t)position[i1-1].z;

				vert.u = has_uvs ? (NMath::scalar_t)uv_coord[i2-1].x : 0.0f;
				vert.v = has_uvs ? (NMath::scalar_t)uv_coord[i2-1].y : 0.0f;

				vert.nx = has_normals ? (NMath::scalar_t)normal[i3-1].x : 0.0f;
				vert.ny = has_normals ? (NMath::scalar_t)normal[i3-1].y : 0.0f;
				vert.nz = has_normals ? (NMath::scalar_t)normal[i3-1].z : 0.0f;

				vertices.push_back(vert);
				indices.push_back(idx);

				// 2nd vertex
				vert.px = (NMath::scalar_t)position[i4-1].x;
				vert.py = (NMath::scalar_t)position[i4-1].y;
				vert.pz = (NMath::scalar_t)position[i4-1].z;

				vert.u = has_uvs ? (NMath::scalar_t)uv_coord[i5-1].x : 0.0f;
				vert.v = has_uvs ? (NMath::scalar_t)uv_coord[i5-1].y : 0.0f;

				vert.nx = has_normals ? (NMath::scalar_t)normal[i6-1].x : 0.0f;
				vert.ny = has_normals ? (NMath::scalar_t)normal[i6-1].y : 0.0f;
				vert.nz = has_normals ? (NMath::scalar_t)normal[i6-1].z : 0.0f;

				vertices.push_back(vert);
				indices.push_back(idx+1);

				// 3rd vertex
				vert.px = (NMath::scalar_t)position[i7-1].x;
				vert.py = (NMath::scalar_t)position[i7-1].y;
				vert.pz = (NMath::scalar_t)position[i7-1].z;

				vert.u = has_uvs ? (NMath::scalar_t)uv_coord[i8-1].x : 0.0f;
				vert.v = has_uvs ? (NMath::scalar_t)uv_coord[i8-1].y : 0.0f;

				vert.nx = has_normals ? (NMath::scalar_t)normal[i9-1].x : 0.0f;
				vert.ny = has_normals ? (NMath::scalar_t)normal[i9-1].y : 0.0f;
				vert.nz = has_normals ? (NMath::scalar_t)normal[i9-1].z : 0.0f;

				vertices.push_back(vert);
				indices.push_back(idx+2);

				idx += 3;
				break;
			}
		}

		if (syntax_error) {
			fclose(fp);
			return 4;
		}

		// Zero out the buffer.
		memset(line, '\0', sizeof(line) * sizeof(char));
	}

	fclose(fp);

	// Update the data in the mesh buffers.
	if(mesh.init(vertices.size(), indices.size()))
		return 5;

	Buffer<vertex_t> &p_meshv = mesh.vertices();
	Buffer<index_t>  &p_meshi = mesh.indices();

	for (unsigned int i = 0; i < vertices.size(); i++)
		p_meshv[i] = vertices[i];

	for (unsigned int i = 0; i < indices.size(); i++)
		p_meshi[i] = indices[i];

	return 0;
}

			} /* namespace Import */

			namespace Export {
			} /* namespace Export */

		} /* namespace IO */
} /* namespace NMesh */
