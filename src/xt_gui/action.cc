#include <thread>
#include <xtcore/log.h>
#include "action.h"

namespace action {

void task_render(workspace_t *ws)
{
    if (ws) {
        ws->context.init();
        ws->setup_callbacks();
        ws->renderer->setup(ws->context);
        ws->setup_callbacks();
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

void not_yet_implemented()
{
    Log::handle().post_warning("This feature is not yet implemented.");
}

} /* namespace action */
