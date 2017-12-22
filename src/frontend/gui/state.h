#ifndef XTGUI_GUI_STATE_H
#define XTGUI_GUI_STATE_H

#include <vector>
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

    std::vector<workspace_t*> workspaces;
    state_t();
};

} /* namespace gui */

#endif /* XTGUI_GUI_STATE_H */
