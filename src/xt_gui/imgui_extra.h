#ifndef XTGUI_IMGUI_EXTRA_H_INCLUDED
#define XTGUI_IMGUI_EXTRA_H_INCLUDED

#include <limits>
#include <imgui/imgui.h>

void slider_float (const char *name, float  &val, float  a, float  b);
void slider_int   (const char *name, size_t &val, size_t a, size_t b);

void textedit_float (const char *name, float  &val, float step, float lim_min = 0.f, float lim_max = std::numeric_limits<float>::max());
void textedit_int   (const char *name, size_t &val, int   step, int   lim_mix = 0.f, int   lim_max = std::numeric_limits<int>::max());

void not_yet_implemented();

#endif /* XTGUI_IMGUI_EXTRA_H_INCLUDED */
