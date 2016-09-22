#include <float.h>
#include <stdint.h>

#include "pixmap.h"

namespace NImg {
    namespace Painter {

typedef struct {
    float x, y, z;
} vec3_t;

typedef struct {
    vec3_t v;
    ColorRGBf c;
} vertex_t;

typedef struct {
    vertex_t a, b, c;
} triangle_t;

typedef struct {
    vertex_t a, b;
} edge_t;

void draw_line(Pixmap &map,
          ColorRGBf &c0, float x0, float y0,
          ColorRGBf &c1, float x1, float y1);

void draw_triangle(Pixmap &map, triangle_t tri);
void fill_triangle(Pixmap &map, triangle_t tri);

    } /* namespace Painter */
} /* namespace NImg */
