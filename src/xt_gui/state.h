#ifndef XTRACER_GUI_STATE_H
#define XTRACER_GUI_STATE_H

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
    int width;
    int height;

    window_t()
        : width(WINDOW_DEFAULT_WIDTH)
        , height(WINDOW_DEFAULT_HEIGHT)
    {}
};

struct state_t
{
    textures_t textures;
    window_t   window;
};

} /* namespace gui */

#endif /* XTRACER_GUI_STATE_H */
