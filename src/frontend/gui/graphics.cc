#include <cstring>
#include <cstdio>
#define NANOSVG_IMPLEMENTATION
#include "nanosvg/nanosvg.h"
#include <imgui/imgui.h>

namespace gui {
    namespace graphics {

typedef struct {
    NSVGimage *img;
} graphic_t;

int load(graphic_t *vec, const char *src, float size)
{
    if (!vec) return -1;
    size_t length = strlen(src);
    char *copy = (char*)malloc(sizeof(char) * (length + 1 ) );
    for (size_t i = 0; i < length; ++i) copy[i] = (char)(src[i]);
    copy[length] = 0;
    vec->img = nsvgParse(copy, "px", size);
    free(copy);
    return 0;
}

int kill(graphic_t *vec)
{
    if (!vec) return -1;
    nsvgDelete(vec->img);
    return 0;
}

void draw(NSVGimage *image, float position_x, float position_y)
{
    ImVec2 pos = ImVec2(position_x, position_y);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    for (NSVGshape *shape = image->shapes; shape != NULL; shape = shape->next) {
	    for (NSVGpath *path = shape->paths; path != NULL; path = path->next) {
    		for (size_t i = 0; i < path->npts-1; i += 3) {
	    		float* p = &path->pts[i*2];

                ImColor col = ImColor(shape->stroke.color);
                col.Value.w = shape->opacity * 255.f;

                draw_list->AddBezierCurve(ImVec2(pos.x + p[0], pos.y + p[1])
                                         ,ImVec2(pos.x + p[2], pos.y + p[3])
                                         ,ImVec2(pos.x + p[4], pos.y + p[5])
                                         ,ImVec2(pos.x + p[6], pos.y + p[7])
                                         ,col, shape->strokeWidth, 4);
    		}
	    }
    }

    ImGui::InvisibleButton("", ImVec2(image->width, image->height));
}

    } /* namespace graphics */
} /* namespace gui */
