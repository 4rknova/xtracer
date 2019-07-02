#ifndef XT_GUI_H_INCLUDED
#define XT_GUI_H_INCLUDED

#include "workspace.h"
#include "state.h"

namespace gui {

void mm_tileorder   (workspace_t *ws);
void mm_resolution  (workspace_t *ws);
void mm_export      (workspace_t *ws);
void mm_renderer    (workspace_t *ws);
void mm_zoom        (workspace_t *ws);

void render_main_menu(state_t *state);

void container(state_t *state);

void init(state_t *state);
void draw_widgets(state_t *state);

} /* namespace gui */

#endif /* XT_GUI_H_INCLUDED */
