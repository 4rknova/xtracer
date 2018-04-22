#ifndef XT_GRAPHICS_H_INCLUDED
#define XT_GRAPHICS_H_INCLUDED

namespace gui {
    namespace graphics {

enum GRAPHIC {
      GRAPHIC_LOGO
      // Icons
    , GRAPHIC_CAMERA
    , GRAPHIC_OBJECT
    , GRAPHIC_TEXTURE
    , GRAPHIC_COLOR
    , GRAPHIC_VALUE
};

void init();
void deinit();
void draw(GRAPHIC graphic, float position_x, float position_y);

    } /* namespace graphics */
} /* namespace gui */


#endif /* XT_GRAPHICS_H_INCLUDED */
