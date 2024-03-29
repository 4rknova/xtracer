#include <xtcore/memutil.tml>
#include <xtcore/parseutil.h>
#include "workspace.h"

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
    if (clear_buffer) {
        float *data = new float[context.params.width * context.params.height*4];
        memset(data, 0, sizeof(float) * 4 * context.params.width * context.params.height);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, context.params.width, context.params.height, 0, GL_RGBA, GL_FLOAT, data);
        delete[] data;
    }
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
    return status == WS_STATUS_PROCESSING;
}

void workspace_t::update()
{
    float pu = 1.f/context.tiles.size();

    m.lock();

    glBindTexture(GL_TEXTURE_2D, texture);

    if (show_tile_updates) {
    	while (1) {
            xtcore::render::tile_t *t = handler_init.pop();

            if (!t) break;

    		for (size_t y = t->y0(); y < t->y1(); ++y) {
                for (size_t x = t->x0(); x < t->x1(); ++x) {
 //                   nimg::ColorRGBAf col;
 //                  t->read(x, y, col);
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
                float data[4] = {col.r(), col.g(), col.b(), col.a()};
                glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, 1, 1, GL_RGBA, GL_FLOAT, data);
            }
		}
        progress += pu;
	}
    m.unlock();
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
    glDeleteTextures(1, &texture);
}
