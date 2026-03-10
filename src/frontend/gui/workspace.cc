#include <xtcore/memutil.tml>
#include <xtcore/parseutil.h>
#include <nimg/conversion.h>
#include "workspace.h"

namespace {

bool is_realtime_gl_integrator(const xtcore::render::IIntegrator *integrator)
{
#if defined(XTRACER_ENABLE_REALTIME_GL) && XTRACER_ENABLE_REALTIME_GL
    return dynamic_cast<const xtcore::integrator::realtime_gl::Integrator*>(integrator) != nullptr;
#else
    (void)integrator;
    return false;
#endif
}

} // namespace

ws_handler_t::ws_handler_t(std::mutex *m)
    : mut(m)
{}

void ws_handler_t::handle_event(xtcore::render::tile_t *tile)
{
	mut->lock();
	tiles.push(tile);
    mut->unlock();
}

xtcore::render::tile_t *ws_handler_t::pop()
{
    xtcore::render::tile_t *t = nullptr;

    if (tiles.size() > 0) {
        t = tiles.front();
        tiles.pop();
    }

    return t;
}

void workspace_t::load()
{
    int err = xtcore::io::scn::load(&(context.scene), source_file.c_str());
    status = (err ? WS_STATUS_INVALID : WS_STATUS_LOADED);

    // Auto-select camera
    {
        xtcore::render::params_t *p = &(context.params);
        auto first_cam = context.scene.m_cameras.begin();
        bool is_cam_valid = (first_cam != context.scene.m_cameras.end());
        if (is_cam_valid && (p->camera == HASH_ID_INVALID)) p->camera = (*first_cam).first;
    }

    gui::graph::build(&graph, &(context.scene));
}

void workspace_t::prepare()
{
    const size_t pixel_count = context.params.width * context.params.height;
    float *data = nullptr;
    if (clear_buffer) {
        data = new float[pixel_count * 4];
        memset(data, 0, sizeof(float) * 4 * pixel_count);
    }
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, context.params.width, context.params.height, 0, GL_RGBA, GL_FLOAT, data);
    if (data) delete[] data;
}

void workspace_t::render()
{
    status   = WS_STATUS_PROCESSING;
    progress = 0.f;

    context.init();
    setup_callbacks();
    integrator->setup(context);
    xtcore::render::order(context.tiles, context.params.tile_order);

    switch (rmode) {
        case WS_RMODE_SINGLE:
        {
            timer.start();
            integrator->render();
            timer.stop();
        } break;
        case WS_RMODE_CONTINUOUS:
        {
            while (rmode == WS_RMODE_CONTINUOUS) {
                integrator->render();
            }
        } break;
    }

    status = WS_STATUS_LOADED;
}

void workspace_t::setup_callbacks()
{
    progress = 0.f;
    for (auto& i : context.tiles) {
        i.setup_handler_on_init(&handler_init);
        i.setup_handler_on_done(&handler_done);
    }
}

bool workspace_t::is_idle()
{
    return status == WS_STATUS_LOADED;
}

bool workspace_t::is_rendering()
{
    return status == WS_STATUS_PROCESSING || realtime_gl_active;
}

void workspace_t::update()
{
    if (realtime_gl_active) {
        render_realtime_gl_frame();
        return;
    }

    float pu = 1.f/context.tiles.size();

    m.lock();

    glBindTexture(GL_TEXTURE_2D, texture);

    if (show_tile_updates) {
    	while (1) {
            xtcore::render::tile_t *t = handler_init.pop();

            if (!t) break;

    		for (size_t y = t->y0(); y < t->y1(); ++y) {
                for (size_t x = t->x0(); x < t->x1(); ++x) {
                    float data[4] = {1,0,0,1};
                    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, 1, 1, GL_RGBA, GL_FLOAT, data);
                }
    		}
    	}
    }

    while (1) {
        xtcore::render::tile_t *t = handler_done.pop();

        if (!t) { break; }

		for (size_t y = t->y0(); y < t->y1(); ++y) {
            for (size_t x = t->x0(); x < t->x1(); ++x) {
                nimg::ColorRGBAf col;
                t->read(x, y, col);
                float data[4] = {
                    linear_to_srgb(col.r()),
                    linear_to_srgb(col.g()),
                    linear_to_srgb(col.b()),
                    col.a()
                };
                glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, 1, 1, GL_RGBA, GL_FLOAT, data);
            }
		}
        progress += pu;
	}
    m.unlock();
}

bool workspace_t::uses_realtime_gl() const
{
    return is_realtime_gl_integrator(integrator);
}

void workspace_t::stop_realtime_gl()
{
    if (gui_renderer) {
        gui::destroy_gui_renderer(gui_renderer, this);
        gui_renderer = 0;
    }
    realtime_gl_active = false;
}

void workspace_t::start_realtime_gl()
{
#if !(defined(XTRACER_ENABLE_REALTIME_GL) && XTRACER_ENABLE_REALTIME_GL)
    return;
#else
    if (!uses_realtime_gl()) return;

    context.init();
    status = WS_STATUS_LOADED;
    progress = 0.f;
    timer.start();

    if (gui_renderer) {
        gui::destroy_gui_renderer(gui_renderer, this);
        gui_renderer = 0;
    }
    gui_renderer = gui::create_gui_renderer(this);
    if (!gui_renderer) return;
    if (!gui_renderer->begin(this)) {
        gui::destroy_gui_renderer(gui_renderer, this);
        gui_renderer = 0;
        return;
    }

    if (rmode == WS_RMODE_SINGLE) {
        render_realtime_gl_frame();
        timer.stop();
        progress = 1.f;
        realtime_gl_active = false;
    } else {
        realtime_gl_active = true;
    }
#endif
}

void workspace_t::render_realtime_gl_frame()
{
#if !(defined(XTRACER_ENABLE_REALTIME_GL) && XTRACER_ENABLE_REALTIME_GL)
    return;
#else
    if (!gui_renderer) return;
    gui_renderer->render_frame(this);

    progress = 1.f;
#endif
}

workspace_t::workspace_t()
    : status(WS_STATUS_INVALID)
    , texture(0)
    , zoom_multiplier(1.001f)
    , integrator(0)
    , handler_init(&m)
    , handler_done(&m)
    , gamma(DEFAULT_GAMMA)
    , clear_buffer(true)
    , show_tile_updates(true)
    , rmode(WS_RMODE_SINGLE)
    , realtime_gl_active(false)
    , gui_renderer(0)
{
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S    , GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T    , GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
}

workspace_t::~workspace_t()
{
    stop_realtime_gl();
    glDeleteTextures(1, &texture);
}
