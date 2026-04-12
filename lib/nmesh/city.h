#ifndef NMESH_CITY_H_INCLUDED
#define NMESH_CITY_H_INCLUDED

#include "structs.h"

namespace nmesh {
    namespace generator {

void city(object_t *obj,
          int   seed                   = 1337,
          int   blocks_x               = 4,
          int   blocks_z               = 4,
          float block_size             = 1.0f,
          float road_width             = 0.15f,
          float building_height_min    = 0.1f,
          float building_height_max    = 0.8f,
          float lot_padding            = 0.04f,
          int   buildings_per_block_x  = 2,
          int   buildings_per_block_z  = 2,
          float floor_height           = 0.22f,
          float bay_width              = 0.22f,
          float window_width_ratio     = 0.55f,
          float window_height_ratio    = 0.55f,
          float window_inset           = 0.04f,
          float pavement_height        = 0.012f,
          float pavement_width         = 0.04f);

    } /* namespace generator */
} /* namespace nmesh */

#endif /* NMESH_CITY_H_INCLUDED */
