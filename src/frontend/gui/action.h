#ifndef XTGUI_GUI_ACTION_H_INCLUDED
#define XTGUI_GUI_ACTION_H_INCLUDED

#include "state.h"
#include "workspace.h"

namespace action {

enum IMG_FORMAT
{
      IMG_FORMAT_HDR
    , IMG_FORMAT_PNG
    , IMG_FORMAT_JPG
    , IMG_FORMAT_BMP
    , IMG_FORMAT_TGA
};

/* networking actions */
int broadcast (gui::state_t *state);
int listen    (gui::state_t *state);

/* rendering actions */
int render (workspace_t *ws);

/* auxiliary actions */
int load   (workspace_t *ws);
int close  (gui::state_t *state, workspace_t *ws);

int write(IMG_FORMAT format, const char *filepath, workspace_t *ws);

void quit();

} /* namespace action */

#endif /* XTGUI_GUI_ACTION_H_INCLUDED */
