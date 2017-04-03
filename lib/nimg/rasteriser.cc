#include <float.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include "rasteriser.h"

namespace nimg {
    namespace painter {

void draw_line(Pixmap &map,
         ColorRGBAf &c0, float x0, float y0,
         ColorRGBAf &c1, float x1, float y1)
{
	float xdiff = (x1 - x0);
	float ydiff = (y1 - y0);
    float absxd = fabs(xdiff);
    float absyd = fabs(ydiff);

	if(absxd < FLT_EPSILON && absyd < FLT_EPSILON) {
		map.pixel((unsigned int)x0, (unsigned int)y0) = c0;
		return;
	}

	if (absxd > absyd) {
		float xmin, xmax;

		if (x0 < x1) { xmin = x0; xmax = x1; }
        else         { xmin = x1; xmax = x0; }

		/* draw line in terms of y slope */
		float slope = ydiff / xdiff;
		for (float x = xmin; x <= xmax; x += 1.f) {
			float y = y1 + ((x - x1) * slope);
            float t = (x - x0) / xdiff;
            ColorRGBAf col = c1 * t + c0 * (1.f - t);
			map.pixel((unsigned int)x, (unsigned int)y) = col;
		}

	} else {
		float ymin, ymax;

		// set ymin to the lower y value given
		// and ymax to the higher value
		if (y0 < y1) { ymin = y0; ymax = y1; }
        else         { ymin = y1; ymax = y0; }

		// draw line in terms of x slope
		float slope = xdiff / ydiff;
		for (float y = ymin; y <= ymax; y += 1.f) {
			float x = x0 + ((y - y0) * slope);
            float t = (y - y0) / ydiff;
			ColorRGBAf col = c1 * t + c0 * (1.f - t);
			map.pixel((unsigned int)x, (unsigned int)y) = col;
		}
	}
}

void draw_triangle(Pixmap &map, triangle_t tri)
{
    draw_line(map, tri.a.c, tri.a.v.x, tri.a.v.y
                 , tri.b.c, tri.b.v.x, tri.b.v.y);
    draw_line(map, tri.b.c, tri.b.v.x, tri.b.v.y
                 , tri.c.c, tri.c.v.x, tri.c.v.y);
    draw_line(map, tri.c.c, tri.c.v.x, tri.c.v.y
                 , tri.a.c, tri.a.v.x, tri.a.v.y);
}

void draw_span(Pixmap &map, int x0, int x1, ColorRGBAf c0, ColorRGBAf c1, int y)
{
	int xdiff = x1 - x0;

    if(xdiff == 0) return;

    ColorRGBAf colordiff = c1 - c0;

	float factor = 0.f;
    float factorStep = 1.f / (float)xdiff;

	// draw each pixel in the span
    for (int x = x0; x < x1; ++x) {
        ColorRGBAf src = map.pixel_ro(x,y);
        ColorRGBAf res = c0 + colordiff * factor;

        map.pixel(x,y) = ColorRGBf(src) * (1.-res.a()) + ColorRGBf(res) * res.a();
        factor += factorStep;
    }
}


void draw_spans(Pixmap &map, edge_t e0, edge_t e1)
{
	// calculate difference between the y coordinates
    // of the first edge and return if 0
    float e1ydiff = (float)(e0.b.v.y - e0.a.v.y);
    if(e1ydiff == 0.0f) return;
	// calculate difference between the y coordinates
    // of the second edge and return if 0
    float e2ydiff = (float)(e1.b.v.y - e1.a.v.y);
    if(e2ydiff == 0.0f) return;

	// calculate differences between the x coordinates
    // and colors of the points of the edges
    float e1xdiff = (float)(e0.b.v.x - e0.a.v.x);
    float e2xdiff = (float)(e1.b.v.x - e1.a.v.x);
    ColorRGBAf e1colordiff = e0.b.c - e0.a.c;
    ColorRGBAf e2colordiff = e1.b.c - e1.a.c;

	// calculate factors to use for interpolation
    // with the edges and the step values to increase
    // them by after drawing each span
    float factor1 = (float)(e1.a.v.y - e0.a.v.y) / e1ydiff;
    float factorStep1 = 1.0f / e1ydiff;
    float factor2 = 0.0f;
    float factorStep2 = 1.0f / e2ydiff;

	// loop through the lines between the edges and draw spans
    for(int y = e1.a.v.y; y < e1.b.v.y; ++y) {
        // create and draw span
        int x0 = e0.a.v.x + (int)(e1xdiff * factor1);
        int x1 = e1.a.v.x + (int)(e2xdiff * factor2);
        ColorRGBAf c0 = e0.a.c + e1colordiff * factor1;
        ColorRGBAf c1 = e1.a.c + e2colordiff * factor2;

        if (x0 > x1) {
            int tv = x0;
            ColorRGBAf tc = c0;

            x0 = x1;
            x1 = tv;
            c0 = c1;
            c1 = tc;
        }

        draw_span(map, x0, x1, c0, c1, y);

        // increase factors
        factor1 += factorStep1;
        factor2 += factorStep2;
    }
}

void fill_triangle(Pixmap &map, triangle_t tri)
{
    edge_t edges[3] = {
        { tri.a, tri.b },
        { tri.b, tri.c },
        { tri.c, tri.a }
    };

	int maxLength = 0;
    int longEdge = 0;

    for (int i = 0; i < 3; ++i) {
		if (edges[i].a.v.y > edges[i].b.v.y) {
			vertex_t t = edges[i].a;
			edges[i].a = edges[i].b;
			edges[i].b = t;
    	}

		int length = edges[i].b.v.y - edges[i].a.v.y;

		if (length > maxLength) {
			maxLength = length;
			longEdge = i;
		}
    }

	int shortEdge1 = (longEdge + 1) % 3;
    int shortEdge2 = (longEdge + 2) % 3;

    // draw spans between edges; the long edge can be drawn
    // with the shorter edges to draw the full triangle
    draw_spans(map, edges[longEdge], edges[shortEdge1]);
    draw_spans(map, edges[longEdge], edges[shortEdge2]);
}

    } /* namespace painter */
} /* namespace nimg */
