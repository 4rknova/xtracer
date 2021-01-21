#include <thread>
#include <string>
#include <nimg/img.h>
#include <xtcore/context.h>
#include <xtcore/log.h>
#include <xtcore/timeutil.h>
#include <xtcore/macro.h>
#include "action.h"

namespace action {

void task_render(workspace_t *ws)
{
    if (!ws) return;
    ws->render();
    std::string timestr;
    print_time_breakdown(timestr, ws->timer.get_time_in_mlsec());
    xtcore::Log::handle().post_message("Render completed: [%s] -> %s", ws->source_file.c_str(), timestr.c_str());
}

void task_load(workspace_t *ws)
{
    if (ws) ws->load();
}

int render(workspace_t *ws)
{
    if (!ws) return 1;

    ws->prepare();
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
            delete ws;
            break;
        }
    }

    return 0;
}

int write(IMG_FORMAT format, const char *filepath, workspace_t *ws)
{
    std::string fp = filepath;
    nimg::Pixmap fb;
    xtcore::render::assemble(fb, ws->context);
    xtcore::Log::handle().post_message("Exporting %s..", fp.c_str());

    int res = 1;

    switch (format) {
        case IMG_FORMAT_EXR: fp += ".exr"; res = nimg::io::save::exr(fp.c_str(), fb); break;
        case IMG_FORMAT_HDR: fp += ".hdr"; res = nimg::io::save::hdr(fp.c_str(), fb); break;
        case IMG_FORMAT_PNG: fp += ".png"; res = nimg::io::save::png(fp.c_str(), fb); break;
        case IMG_FORMAT_JPG: fp += ".jpg"; res = nimg::io::save::jpg(fp.c_str(), fb); break;
        case IMG_FORMAT_BMP: fp += ".bmp"; res = nimg::io::save::bmp(fp.c_str(), fb); break;
        case IMG_FORMAT_TGA: fp += ".tga"; res = nimg::io::save::tga(fp.c_str(), fb); break;
    }

    if (res) xtcore::Log::handle().post_error("Failed to export %s", fp.c_str());
    return res;
}

} /* namespace action */
