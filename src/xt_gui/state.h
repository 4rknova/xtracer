#ifndef XTGUI_GUI_STATE_H
#define XTGUI_GUI_STATE_H

#include "opengl.h"
#include "workspace.h"

namespace gui {

struct textures_t
{
    GLuint logo;
    GLuint render;

    textures_t();
};

struct window_t
{
    int width;
    int height;

    window_t();
};

struct state_t
{
    textures_t   textures;
    window_t     window;
    workspace_t *workspace;

    state_t();
};

} /* namespace gui */

#endif /* XTGUI_GUI_STATE_H */
