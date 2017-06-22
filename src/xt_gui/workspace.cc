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

void workspace_t::init()
{
    progress = 0.f;
}

void workspace_t::init_texture()
{
    if (!texture) {
        glGenTextures(1, &texture);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }
    glBindTexture(GL_TEXTURE_2D, texture);
    float *data = new float[context.params.width * context.params.height*4];
    memset(data, 0, sizeof(float) * 4 * context.params.width * context.params.height);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, context.params.width, context.params.height, 0, GL_RGBA, GL_FLOAT, data);
    delete data;
}

void workspace_t::setup_callbacks()
{
    progress = 0.f;
    for (auto& i : context.tiles) {
        i.setup_handler_on_init   (&handler_init);
        i.setup_handler_on_done   (&handler_done);
    }
}

void workspace_t::deinit()
{
    glDeleteTextures(1, &texture);
}

void workspace_t::update()
{
    float pu = 1.f/context.tiles.size();

    m.lock();

    glBindTexture(GL_TEXTURE_2D, texture);
	while (1) {
        xtcore::render::tile_t *t = handler_init.pop();

        if (!t) break;

		for (int y = t->y0(); y < t->y1(); ++y) {
            for (int x = t->x0(); x < t->x1(); ++x) {
                nimg::ColorRGBAf col;
                t->read(x, y, col);
                float data[4] = {1,0,0,1};
                glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, 1, 1, GL_RGBA, GL_FLOAT, data);
            }
		}
	}

    while (1) {
        xtcore::render::tile_t *t = handler_done.pop();

        if (!t) break;

		for (int y = t->y0(); y < t->y1(); ++y) {
            for (int x = t->x0(); x < t->x1(); ++x) {
                nimg::ColorRGBAf col;
                t->read(x, y, col);
                float data[4] = {col.r(), col.g(), col.b(), col.a()};
                // Apply gamma correction
                nmath_pow(data[0], gamma);
                nmath_pow(data[1], gamma);
                nmath_pow(data[2], gamma);
                glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, 1, 1, GL_RGBA, GL_FLOAT, data);
            }
		}
        progress += pu;
	}
    m.unlock();
}
