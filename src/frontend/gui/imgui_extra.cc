#include <string>
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

void textedit_float(const char *name, float &val, float step, float lim_min, float lim_max)
{
    float tmp = val;
    if (ImGui::InputFloat(name, &tmp, step, 2.f * step, -1, ImGuiInputTextFlags_EnterReturnsTrue)) {
        CLAMP(tmp, lim_min, lim_max);
        val = tmp;
    }
}

void textedit_double(const char *name, double &val, float step, float lim_min, float lim_max)
{
    float tmp = (float)val;
    if (ImGui::InputFloat(name, &tmp, step, 2.f * step, -1, ImGuiInputTextFlags_EnterReturnsTrue)) {
        CLAMP(tmp, lim_min, lim_max);
        val = (double)tmp;
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

void textedit_float2(const char *name, NMath::Vector2f &vec, float step)
{
    float x = vec.x;
    float y = vec.y;

    std::string nx(name); nx.append(".x");
    std::string ny(name); ny.append(".y");

    ImGuiInputTextFlags f = ImGuiInputTextFlags_EnterReturnsTrue;

    if (ImGui::InputFloat(nx.c_str(), &x, step, 2 * step, f)) vec.x = x;
    if (ImGui::InputFloat(ny.c_str(), &y, step, 2 * step, f)) vec.y = y;
}

void textedit_float3(const char *name, NMath::Vector3f &vec, float step)
{
    float x = vec.x;
    float y = vec.y;
    float z = vec.z;

    std::string nx(name); nx.append(".x");
    std::string ny(name); ny.append(".y");
    std::string nz(name); nz.append(".z");

    ImGuiInputTextFlags f = ImGuiInputTextFlags_EnterReturnsTrue;

    if (ImGui::InputFloat(nx.c_str(), &x, step, 2 * step, f)) vec.x = x;
    if (ImGui::InputFloat(ny.c_str(), &y, step, 2 * step, f)) vec.y = y;
    if (ImGui::InputFloat(nz.c_str(), &z, step, 2 * step, f)) vec.z = z;
}

void not_yet_implemented()
{
    ImGui::Text("Not yet implemented");
}

namespace ImGui {
    bool GoxTab(const char *text, float width, bool *v)
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
        text_size.x = MAX(text_size.x, width);
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
            pos.y -= glyph->AdvanceX;
        }
        ImGui::PopID();
        return ret;
}
} // namespace ImGui
