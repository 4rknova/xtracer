#ifndef NIMG_RASTERISER_H_INCLUDED
#define NIMG_RASTERISER_H_INCLUDED

#include "pixmap.h"

namespace nimg {
    namespace painter {

typedef struct {
    float x, y, z;
} vec3_t;

typedef struct {
    vec3_t v;
    ColorRGBAf c;
} vertex_t;

typedef struct {
    vertex_t a, b, c;
} triangle_t;

typedef struct {
    vertex_t a, b;
} edge_t;

void draw_line(Pixmap &map,
          ColorRGBAf &c0, float x0, float y0,
          ColorRGBAf &c1, float x1, float y1);

void draw_triangle(Pixmap &map, triangle_t tri);
void fill_triangle(Pixmap &map, triangle_t tri);

    } /* namespace painter */
} /* namespace nimg */

#endif /* NIMG_RASTERISER_H_INCLUDED */
