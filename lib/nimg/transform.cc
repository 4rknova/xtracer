#include "transform.h"

namespace nimg {
    namespace transform {

bool flip_horizontal(Pixmap *map) {
    if (!map) return false;

    size_t w = map->width();
    size_t h = map->height();

    for (size_t j = 0; j < h; ++j) {
        for (size_t i = 0; i < w / 2; ++i) {
            size_t k = w - 1 - i;
            ColorRGBAf val_a = map->pixel_ro(i, j);
            ColorRGBAf val_b = map->pixel_ro(k, j);
            map->pixel(i,j) = val_b;
            map->pixel(k,j) = val_a;
        }
    }

    return true;
}

bool flip_vertical(Pixmap *map) {
    if (!map) return false;

    size_t w = map->width();
    size_t h = map->height();

    for (size_t j = 0; j < h / 2; ++j) {
        for (size_t i = 0; i < w; ++i) {
            size_t k = h - 1 - j;
            ColorRGBAf val_a = map->pixel_ro(i, j);
            ColorRGBAf val_b = map->pixel_ro(i, k);
            map->pixel(i,j) = val_b;
            map->pixel(i,k) = val_a;
        }
    }
    return true;
}

    } /* namespace transform */
} /* namespace nimg */
