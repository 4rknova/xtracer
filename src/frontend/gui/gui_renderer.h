#ifndef XTGUI_GUI_RENDERER_H_INCLUDED
#define XTGUI_GUI_RENDERER_H_INCLUDED

struct workspace_t;

namespace gui {

class IGuiRenderer
{
    public:
    virtual ~IGuiRenderer() {}
    virtual bool begin(::workspace_t *ws) = 0;
    virtual void render_frame(::workspace_t *ws) = 0;
    virtual void end(::workspace_t *ws) = 0;
};

IGuiRenderer *create_gui_renderer(::workspace_t *ws);
void destroy_gui_renderer(IGuiRenderer *renderer, ::workspace_t *ws);

} // namespace gui

#endif /* XTGUI_GUI_RENDERER_H_INCLUDED */
