#ifndef QUASAR_GUI_WIDGERS_H
#define QUASAR_GUI_WIDGETS_H

#include <queue>
#include <xtcore/context.h>
#include "state.h"

namespace gui {

enum ACTION
{
      ACTION_NOP
    , ACTION_RENDER
    , ACTION_SCENE_SAVE
    , ACTION_SCENE_LOAD
    , ACTION_RENDER_SAVE
};

void init(state_t *state);
ACTION get_action();
void handle_io_kb(unsigned char key);
void handle_io_ms(int x, int y, bool button_event = false
                    , bool left   = false
                    , bool right  = false
                    , float wheel = 0.f);

void draw_widgets(state_t *state);

} /* namespace gui */

#endif /* QUASAR_GUI_WIDGETS_H */
