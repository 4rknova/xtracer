#include "workspace.h"

ws_handler_t::ws_handler_t(std::mutex *m)
    : mut(m)
{}

void ws_handler_t::handle_event(xtracer::render::tile_t *tile)
{
	mut->lock();
	tiles.push(tile);
    mut->unlock();
}

xtracer::render::tile_t *ws_handler_t::pop()
{
    xtracer::render::tile_t *t = nullptr;

    if (tiles.size() > 0) {
        t = tiles.front();
        tiles.pop();
    }

    return t;
}

void workspace_t::init()
{
    deinit();

    glGenTextures(1, &texture);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    float *data = new float[context.params.width * context.params.height*4];
    memset(data, 0, sizeof(float) * 4 * context.params.width * context.params.height);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, context.params.width, context.params.height, 0, GL_RGBA, GL_FLOAT, data);
    delete data;
}

void workspace_t::setup_callbacks()
{
    for (auto& i : context.tiles) {
        i.setup_handler_on_init(&handler_init);
        i.setup_handler_on_done(&handler_done);
    }
}

void workspace_t::deinit()
{
    glDeleteTextures(1, &texture);
}

void workspace_t::update()
{
    glBindTexture(GL_TEXTURE_2D, texture);

    m.lock();
	while(1) {
        xtracer::render::tile_t *t = handler_init.pop();

        if (!t) break;

		for (int y = t->y0(); y < t->y1(); ++y) {
            for (int x = t->x0(); x < t->x1(); ++x) {
                nimg::ColorRGBf col;
                t->read(x, y, col);
                float data[4] = {1,0,0,1};
                glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, 1, 1, GL_RGBA, GL_FLOAT, data);
            }
		}
	}

    while(1) {
        xtracer::render::tile_t *t = handler_done.pop();

        if (!t) break;

		for (int y = t->y0(); y < t->y1(); ++y) {
            for (int x = t->x0(); x < t->x1(); ++x) {
                nimg::ColorRGBf col;
                t->read(x, y, col);
                float data[4] = {col.r(), col.g(), col.b(), 1};
                glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, 1, 1, GL_RGBA, GL_FLOAT, data);
            }
		}
	}
    m.unlock();
}
