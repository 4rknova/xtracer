#ifndef XTGUI_GUI_WORKSPACE_H_INCLUDED
#define XTGUI_GUI_WORKSPACE_H_INCLUDED

#include <string>
#include <queue>
#include <mutex>
#include <imgui/imgui.h>
#include <nplatform/timer.h>
#include <xtcore/context.h>
#include <xtcore/tile.h>
#include <xtcore/integrator.h>
#include "graph.h"
#include "opengl.h"

#define DEFAULT_GAMMA       (2.2)

enum WS_STATUS
{
      WS_STATUS_INVALID
    , WS_STATUS_LOADED
    , WS_STATUS_PROCESSING
};

enum WS_RMODE
{
      WS_RMODE_SINGLE
    , WS_RMODE_CONTINUOUS
};

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
    WS_STATUS                    status;
    GLuint                       texture;
    float                        zoom_multiplier;
    std::string                  source_file;
    xtcore::render::IIntegrator *integrator;
    xtcore::render::context_t    context;

    void load();
    void prepare();
    void render();
    void update();
    void setup_callbacks();
    bool is_rendering();

    float progress;

    int block_begin(void *p);
    int block_done(void *p);

    ws_handler_t handler_init;
    ws_handler_t handler_done;

    std::mutex m;

    float gamma;

    Timer timer;

    bool clear_buffer;
    bool show_tile_updates;

    WS_RMODE rmode;

     workspace_t();
    ~workspace_t();


    // Editor
    ImVec2              scroll_position;
    gui::graph::graph_t graph;
};

#endif /* XTGUI_GUI_WORKSPACE_H_INCLUDED */
