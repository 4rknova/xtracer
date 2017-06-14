#ifndef XTGUI_GUI_ACTION_H_INCLUDED
#define XTGUI_GUI_ACTION_H_INCLUDED

#include "workspace.h"

namespace action {

int render(workspace_t *ws);

int load(workspace_t *ws);
int save(workspace_t *ws);

int export_hdr(workspace_t *ws);
int export_png(workspace_t *ws);
int export_bmp(workspace_t *ws);
int export_tga(workspace_t *ws);

void not_yet_implemented();
void quit();

} /* namespace action */

#endif /* XTGUI_GUI_ACTION_H_INCLUDED */
