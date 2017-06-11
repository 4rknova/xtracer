#ifndef XTGUI_GUI_WORKSPACE_H_INCLUDED
#define XTGUI_GUI_WORKSPACE_H_INCLUDED

#include <string>
#include <queue>
#include <mutex>
#include <GL/gl.h>
#include <xtcore/context.h>
#include <xtcore/tile.h>
#include <xtcore/renderer.h>

struct ws_handler_t : public xtcore::render::tile_event_handler_t
{
    ws_handler_t(std::mutex *m);
    std::queue<xtcore::render::tile_t*> tiles;
    std::mutex *mut;
    void handle_event(xtcore::render::tile_t *tile);
    xtcore::render::tile_t *pop();
};

struct workspace_t
{
    GLuint                      texture;
    float                       zoom_multiplier;
    std::string                 source_file;
    xtcore::render::IRenderer *renderer;
    xtcore::render::context_t  context;

    void init();
    void deinit();
    void update();
    void setup_callbacks();

    float progress;

    int block_begin(void *p);
    int block_done(void *p);

    ws_handler_t handler_init;
    ws_handler_t handler_done;

    std::mutex m;

    workspace_t()
        : texture(0)
        , zoom_multiplier(1.f)
        , renderer(0)
        , handler_init(&m)
        , handler_done(&m)
    {}
};

#endif /* XTGUI_GUI_WORKSPACE_H_INCLUDED */
