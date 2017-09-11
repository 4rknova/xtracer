#include "util.h"
#include "imgui_extra.h"
#include <imgui/imgui_internal.h>

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

void not_yet_implemented()
{
    ImGui::Text("Not yet implemented");
}

namespace ImGui {
    bool GoxTab(const char *text, bool *v)
    {
        ImFont *font = GImGui->Font;
        const ImFont::Glyph *glyph;
        char c;
        bool ret;
        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        float pad = style.FramePadding.x;
        ImVec4 color;
        ImVec2 text_size = CalcTextSize(text);
        ImGuiWindow* window = GetCurrentWindow();

        ImVec2 cur = window->DC.CursorPos;

        ImVec2 pos = ImVec2(pad + cur.x, cur.y + text_size.x + pad);

        const  ImU32 text_color = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);
        color = style.Colors[ImGuiCol_Button];
        if (*v) color = style.Colors[ImGuiCol_ButtonActive];
        ImGui::PushStyleColor(ImGuiCol_Button, color);
        ImGui::PushID(text);
        ret = ImGui::Button("", ImVec2(text_size.y + pad * 2,
                                       text_size.x + pad * 2));
        ImGui::PopStyleColor();
        while ((c = *text++)) {
            glyph = font->FindGlyph(c);
            if (!glyph) continue;

            window->DrawList->PrimReserve(6, 4);
            window->DrawList->PrimQuadUV(
                    ImVec2(pos.x + glyph->Y0, pos.y -glyph->X0),
                    ImVec2(pos.x + glyph->Y0, pos.y -glyph->X1),
                    ImVec2(pos.x + glyph->Y1, pos.y -glyph->X1),
                    ImVec2(pos.x + glyph->Y1, pos.y -glyph->X0),

                    ImVec2(glyph->U0, glyph->V0),
                    ImVec2(glyph->U1, glyph->V0),
                    ImVec2(glyph->U1, glyph->V1),
                    ImVec2(glyph->U0, glyph->V1),
                     text_color);
            pos.y -= glyph->XAdvance;
        }
        ImGui::PopID();
        return ret;
}
} // namespace ImGui
