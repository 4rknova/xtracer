#ifndef XTRACER_GUI_WORKSPACE_H_INCLUDED
#define XTRACER_GUI_WORKSPACE_H_INCLUDED

#include <string>
#include <queue>
#include <mutex>
#include <GL/gl.h>
#include <xtcore/context.h>
#include <xtcore/tile.h>
#include <xtcore/renderer.h>

struct ws_handler_t : public xtracer::render::tile_event_handler_t
{
    std::queue<xtracer::render::tile_t*> tiles;
    std::mutex m;
    void handle_event(xtracer::render::tile_t *tile);
    xtracer::render::tile_t *pop();
};

struct workspace_t
{
    GLuint                      texture;
    bool                        is_visible;
    float                       zoom_multiplier;
    std::string                 source_file;
    xtracer::render::IRenderer *renderer;
    xtracer::render::context_t  context;

    void init();
    void deinit();
    void update();
    void setup_callbacks();

    int block_begin(void *p);
    int block_done(void *p);

    ws_handler_t handler_init;
    ws_handler_t handler_done;

    workspace_t()
        : texture(0)
        , is_visible(false)
        , zoom_multiplier(1.f)
        , renderer(0)
    {}
};

#endif /* XTRACER_GUI_WORKSPACE_H_INCLUDED */
