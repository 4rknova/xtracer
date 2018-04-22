#include <cstring>
#include <cstdio>
#define NANOSVG_IMPLEMENTATION
#include "nanosvg/nanosvg.h"
#include <imgui/imgui.h>
#include "graphics.h"
#include "res_icon_camera.h"
#include "res_icon_color.h"
#include "res_logo.h"

namespace gui {
    namespace graphics {

typedef struct {
    NSVGimage *img;
} graphic_t;

graphic_t _resources[6];

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

void init()
{
//    load(&_resources[GRAPHIC_LOGO   ], gui::resources::svg::logo        , 96.f);
//    load(&_resources[GRAPHIC_CAMERA ], gui::resources::svg::icon_camera , 96.f);
//    load(&_resources[GRAPHIC_OBJECT ], gui::resources::svg::icon_object , 96.f);
//   load(&_resources[GRAPHIC_TEXTURE], gui::resources::svg::icon_texture, 96.f);
    load(&_resources[GRAPHIC_COLOR  ], gui::resources::svg::icon_color  , 10.f);
//    load(&_resources[GRAPHIC_VALUE  ], gui::resources::svg::icon_value  , 96.f);

//    load(&icon_svg_cam, logo, 96.f);
}

void deinit()
{
    for (size_t i = 0; i < GRAPHIC_VALUE; ++i) kill(&_resources[i]);
}

void draw(GRAPHIC graphic, float position_x, float position_y)
{
    NSVGimage *image = _resources[graphic].img;
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
