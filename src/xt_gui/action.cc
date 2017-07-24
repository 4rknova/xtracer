#include <thread>
#include <string>
#include <nimg/img.h>
#include <xtcore/context.h>
#include <xtcore/log.h>
#include "action.h"

namespace action {

void task_render(workspace_t *ws)
{
    if (ws) {
        ws->init();
        ws->context.init();
        ws->setup_callbacks();
        ws->renderer->setup(ws->context);
        ws->setup_callbacks();
        xtcore::render::order(ws->context.tiles, ws->tile_order);
        ws->timer.start();
        ws->renderer->render();
        delete ws->renderer;
        ws->renderer = 0;
    }
}

void task_load(workspace_t *ws)
{
    ws->context.scene.load(ws->source_file.c_str(), nullptr);
}

int render(workspace_t *ws)
{
    ws->init_texture();
    std::thread t(task_render, ws);
    t.detach();
    return 0;
}

int load(workspace_t *ws)
{
    std::thread t(task_load, ws);
    t.detach();
    return 0;
}

int save(workspace_t *ws)
{
    not_yet_implemented();
}

void quit()
{
    exit(0);
}

int close(gui::state_t *state, workspace_t *ws)
{
    auto it = state->workspaces.begin()
       , et = state->workspaces.end();

    for (; it != et; ++it) {
        if ((*it) == ws) {
            state->workspaces.erase(it);
            if (state->workspace == ws) state->workspace = 0;
            ws->deinit();
            delete ws;
            break;
        }
    }
}

int export_hdr(const char *filepath, workspace_t *ws)
{
    std::string fp = std::string(filepath) + ".hdr";
    nimg::Pixmap fb;
    xtcore::render::assemble(fb, ws->context);
    Log::handle().post_message("Exporting %s..", fp.c_str());
    int res = nimg::io::save::hdr(fp.c_str(), fb);
    if (res) Log::handle().post_error("Failed to export %d", fp.c_str());
    return res;
}

int export_png(const char *filepath, workspace_t *ws)
{
    std::string fp = std::string(filepath) + ".png";
    nimg::Pixmap fb;
    xtcore::render::assemble(fb, ws->context);
    Log::handle().post_message("Exporting %s..", fp.c_str());
    int res = nimg::io::save::png(fp.c_str(), fb);
    if (res) Log::handle().post_error("Failed to export %d", fp.c_str());
    return res;
}

int export_tga(const char *filepath, workspace_t *ws)
{
    std::string fp = std::string(filepath) + ".tga";
    nimg::Pixmap fb;
    xtcore::render::assemble(fb, ws->context);
    Log::handle().post_message("Exporting %s..", fp.c_str());
    int res = nimg::io::save::tga(fp.c_str(), fb);
    if (res) Log::handle().post_error("Failed to export %d", fp.c_str());
    return res;
}

int export_bmp(const char *filepath, workspace_t *ws)
{
    std::string fp = std::string(filepath) + ".bmp";
    nimg::Pixmap fb;
    xtcore::render::assemble(fb, ws->context);
    Log::handle().post_message("Exporting %s..", fp.c_str());
    int res = nimg::io::save::bmp(fp.c_str(), fb);
    if (res) Log::handle().post_error("Failed to export %d", fp.c_str());
    return res;
}

void not_yet_implemented()
{
    Log::handle().post_warning("This feature is not yet implemented.");
}

} /* namespace action */
