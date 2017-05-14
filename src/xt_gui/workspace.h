#ifndef XTRACER_GUI_WORKSPACE_H_INCLUDED
#define XTRACER_GUI_WORKSPACE_H_INCLUDED

#include <string>
#include <xtcore/context.h>
#include <xtcore/renderer.h>

struct workspace_t
{
    bool                        is_visible;
    float                       zoom_multiplier;
    std::string                 source_file;
    xtracer::render::IRenderer *renderer;
    xtracer::render::context_t  context;

    workspace_t()
        : is_visible(false)
        , zoom_multiplier(1.f)
        , renderer(0)
    {}
};

#endif /* XTRACER_GUI_WORKSPACE_H_INCLUDED */
