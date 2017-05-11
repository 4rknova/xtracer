#ifndef XTRACER_GUI_STATE_H
#define XTRACER_GUI_STATE_H

#include <xtcore/context.h>
#include <xtcore/renderer.h>
#include "opengl.h"

#define WINDOW_DEFAULT_WIDTH  (1920)
#define WINDOW_DEFAULT_HEIGHT (1080)

namespace gui {

struct textures_t
{
    GLuint logo;
    GLuint render;

    textures_t()
        : logo(0)
        , render(0)
    {}
};

struct window_t
{
    size_t width;
    size_t height;

    window_t()
        : width(WINDOW_DEFAULT_WIDTH)
        , height(WINDOW_DEFAULT_HEIGHT)
    {}
};

struct flags_t
{
    bool rendering;

    flags_t()
        : rendering(false)
    {}
};


struct widget_t
{
   bool open;

   widget_t() {
        open = false;
   }
};

struct widgets_t
{
    widget_t main;
    widget_t about;
    widget_t scene_explorer;
    widget_t render;
};

struct state_t
{
    widgets_t  widgets;
    textures_t textures;
    window_t   window;
    flags_t    flags;
    xtracer::render::IRenderer *renderer;
    xtracer::render::context_t ctx;

    state_t()
        : renderer(0)
    {}
};

} /* namespace gui */

#endif /* XTRACER_GUI_STATE_H */
