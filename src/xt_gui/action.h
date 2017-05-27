#ifndef XTGUI_GUI_ACTION_H_INCLUDED
#define XTGUI_GUI_ACTION_H_INCLUDED

#include "workspace.h"

namespace action {

int render(workspace_t *ws);

int load(workspace_t *ws);
int save(workspace_t *ws);

void quit();

void not_yet_implemented();

} /* namespace action */

#endif /* XTGUI_GUI_ACTION_H_INCLUDED */
