#include "util.h"
#include "imgui_extra.h"

void slider_float(const char *name, float &val, float a, float b)
{
    float tmp = val;
    ImGui::SliderFloat(name, &tmp, a, b);
    val = tmp;
}

void slider_int(const char *name, size_t &val, size_t a, size_t b)
{
    int tmp = val;
    ImGui::SliderInt(name, &tmp, a, b);
    val = tmp;
}

void textedit_float(const char *name, float  &val, float step, float lim_min, float lim_max)
{
    float tmp = val;
    if (ImGui::InputFloat("##value", &tmp, step, 2.f * step, -1, ImGuiInputTextFlags_EnterReturnsTrue)) {
        CLAMP(tmp, lim_min, lim_max);
        val = tmp;
    }
}

void textedit_int(const char *name, size_t &val, int step, int lim_min, int lim_max)
{
    int tmp = val;
    if (ImGui::InputInt(name, &tmp, step, 2 * step, ImGuiInputTextFlags_EnterReturnsTrue)) {
        CLAMP(tmp, lim_min, lim_max);
        val = tmp;
    }
}
