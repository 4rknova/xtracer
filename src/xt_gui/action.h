#ifndef XTGUI_GUI_ACTION_H_INCLUDED
#define XTGUI_GUI_ACTION_H_INCLUDED

#include "state.h"
#include "workspace.h"

namespace action {

int render(workspace_t *ws);

int load(workspace_t *ws);
int save(workspace_t *ws);

int close(gui::state_t *state, workspace_t *ws);

int export_hdr(const char *filepath, workspace_t *ws);
int export_png(const char *filepath, workspace_t *ws);
int export_bmp(const char *filepath, workspace_t *ws);
int export_tga(const char *filepath, workspace_t *ws);

void not_yet_implemented();
void quit();

} /* namespace action */

#endif /* XTGUI_GUI_ACTION_H_INCLUDED */
