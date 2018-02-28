#ifndef XTGUI_IMGUI_EXTRA_H_INCLUDED
#define XTGUI_IMGUI_EXTRA_H_INCLUDED

#include <limits>
#include <nmath/vector.h>
#include <imgui/imgui.h>

static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x+rhs.x, lhs.y+rhs.y); }
static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x-rhs.x, lhs.y-rhs.y); }

void slider_float (const char *name, float  &val, float  a, float  b);
void slider_int   (const char *name, size_t &val, size_t a, size_t b);

void textedit_float  (const char *name, float   &val, float  step, float  lim_min = 0.f, float  lim_max = std::numeric_limits<float>::max());
void textedit_double (const char *name, double  &val, float  step, float  lim_min = 0.f, float  lim_max = std::numeric_limits<float>::max());
void textedit_int    (const char *name, size_t  &val, int    step, int    lim_mix = 0  , int    lim_max = std::numeric_limits<int>::max());
void textedit_float2 (const char *name, NMath::Vector2f &vec, float step);
void textedit_float3 (const char *name, NMath::Vector3f &vec, float step);

void not_yet_implemented();

namespace ImGui {
    bool GoxTab(const char *text, float width, bool *v);
}

#endif /* XTGUI_IMGUI_EXTRA_H_INCLUDED */
