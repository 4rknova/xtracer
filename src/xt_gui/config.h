#ifndef XTGUI_CONFIG_H_INCLUDED
#define XTGUI_CONFIG_H_INCLUDED

#define WINDOW_DEFAULT_WIDTH  (1920)
#define WINDOW_DEFAULT_HEIGHT (1080)

#define WIN_FLAGS_SET_0 ( ImGuiWindowFlags_NoCollapse \
                        | ImGuiWindowFlags_NoTitleBar \
                        | ImGuiWindowFlags_NoResize   \
                        | ImGuiWindowFlags_NoMove)

#define WIN_FLAGS_SET_1 WIN_FLAGS_SET_0 \
                        | ImGuiWindowFlags_HorizontalScrollbar


#endif /* XTGUI_CONFIG_H_INCLUDED */
