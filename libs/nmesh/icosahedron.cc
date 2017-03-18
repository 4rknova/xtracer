#include <nmath/precision.h>
#include <nmath/vector.h>
#include "structs.h"
#include "icosahedron.h"

namespace nmesh {
    namespace generator {
// http://blog.andreaskahler.com/2009/06/creating-icosphere-mesh-in-code.html

void icosahedron(object_t *obj)
{
    if (!obj) return;

    // create 12 vertices of a icosahedron
    float k = (1.f + nmath_sqrt(5.f)) / 2.f;

    NMath::Vector3f v[12];

    v[ 0] = NMath::Vector3f(-1,  k,  0).normalized();
    v[ 1] = NMath::Vector3f( 1,  k,  0).normalized();
    v[ 2] = NMath::Vector3f(-1, -k,  0).normalized();
    v[ 3] = NMath::Vector3f( 1, -k,  0).normalized();
    v[ 4] = NMath::Vector3f( 0, -1,  k).normalized();
    v[ 5] = NMath::Vector3f( 0,  1,  k).normalized();
    v[ 6] = NMath::Vector3f( 0, -1, -k).normalized();
    v[ 7] = NMath::Vector3f( 0,  1, -k).normalized();
    v[ 8] = NMath::Vector3f( k,  0, -1).normalized();
    v[ 9] = NMath::Vector3f( k,  0,  1).normalized();
    v[10] = NMath::Vector3f(-k,  0, -1).normalized();
    v[11] = NMath::Vector3f(-k,  0,  1).normalized();

    std::vector<float> *c = &(obj->attributes.v);
    std::vector<float> *n = &(obj->attributes.n);
    std::vector<float> *t = &(obj->attributes.uv);

    for (size_t i = 0; i < 12; ++i) {
        c->push_back(v[i].x);
        c->push_back(v[i].y);
        c->push_back(v[i].z);
        n->push_back(0);
        n->push_back(0);
        n->push_back(0);
        t->push_back(0);
        t->push_back(0);
        t->push_back(0);
    }

    const size_t idx[] = {
           0, 11,  5
         , 0,  5,  1
         , 0,  1,  7
         , 0,  7, 10
         , 0, 10, 11
         , 1,  5,  9
         , 5, 11,  4
         ,11, 10,  2
         ,10,  7,  6
         , 7,  1,  8
         , 3,  9,  4
         , 3,  4,  2
         , 3,  2,  6
         , 3,  6,  8
         , 3,  8,  9
         , 4,  9,  5
         , 2,  4, 11
         , 6,  2, 10
         , 8,  6,  7
         , 9,  8,  1
    };

    nmesh::shape_t shape;
    obj->shapes.push_back(shape);

    for (size_t i = 0; i < 60; ++i) {
        nmesh::index_t f;
        f.v  = idx[i];
        f.n  = idx[i];
        f.uv = idx[i];
        obj->shapes[0].mesh.indices.push_back(f);
    }
}

/*
// refine triangles
for (int i = 0; i < recursionLevel; i++)
{
  var faces2 = new List<TriangleIndices>();
  foreach (var tri in faces)
  {
      // replace triangle by 4 triangles
      int a = getMiddlePoint(tri.v1, tri.v2);
      int b = getMiddlePoint(tri.v2, tri.v3);
      int c = getMiddlePoint(tri.v3, tri.v1);

      faces2.Add(new TriangleIndices(tri.v1, a, c));
      faces2.Add(new TriangleIndices(tri.v2, b, a));
      faces2.Add(new TriangleIndices(tri.v3, c, b));
      faces2.Add(new TriangleIndices(a, b, c));
  }
  faces = faces2;
}

// return index of point in the middle of p1 and p2
    private int getMiddlePoint(int p1, int p2)
    {
        // first check if we have it already
        bool firstIsSmaller = p1 < p2;
        Int64 smallerIndex = firstIsSmaller ? p1 : p2;
        Int64 greaterIndex = firstIsSmaller ? p2 : p1;
        Int64 key = (smallerIndex << 32) + greaterIndex;

        int ret;
        if (this.middlePointIndexCache.TryGetValue(key, out ret))
        {
            return ret;
        }

        // not in cache, calculate it
        Point3D point1 = this.geometry.Positions[p1];
        Point3D point2 = this.geometry.Positions[p2];
        Point3D middle = new Point3D(
            (point1.X + point2.X) / 2.0, 
            (point1.Y + point2.Y) / 2.0, 
            (point1.Z + point2.Z) / 2.0);

        // add vertex makes sure point is on unit sphere
        int i = addVertex(middle); 

        // store it, return index
        this.middlePointIndexCache.Add(key, i);
        return i;
    }

*/
    } /* namespace generator */
} /* namespace nmesh */
