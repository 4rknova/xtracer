#include <cmath>
#include <imgui.h>
#include "graph.h"

static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x+rhs.x, lhs.y+rhs.y); }
static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x-rhs.x, lhs.y-rhs.y); }

void draw_nlist(bool* opened, graph_t *graph)
{
    ImVector<node_t> *nodes = &(graph->nodes);
    ImVector<link_t> *links = &(graph->links);

    ImGui::BeginChild("node_list", ImVec2(100,0));
    ImGui::Text("Nodes");
    ImGui::Separator();

    bool open_context_menu = false;
    int node_hovered_in_list = -1;
    int node_hovered_in_scene = -1;

    for (int node_idx = 0; node_idx < nodes->Size; node_idx++)
    {
        node_t* node = &(nodes->Data[node_idx]);
        ImGui::PushID(node->ID);
        if (ImGui::Selectable(node->Name, node->ID == graph->active_node))
            graph->active_node = node->ID;
        if (ImGui::IsItemHovered())
        {
            node_hovered_in_list = node->ID;
            open_context_menu |= ImGui::IsMouseClicked(1);
        }
        ImGui::PopID();
    }
    ImGui::EndChild();
}

void draw_graph(bool* opened, graph_t *graph, link_t *link)
{
    ImVector<node_t> *nodes = &(graph->nodes);
    ImVector<link_t> *links = &(graph->links);

    static ImVec2 scrolling = ImVec2(0.0f, 0.0f);
    static bool show_grid = true;

    // Draw a list of nodes on the left side
    bool open_context_menu = false;
    int node_hovered_in_list = -1;
    int node_hovered_in_scene = -1;

    ImGui::SameLine();
    ImGui::BeginGroup();

    const float NODE_SLOT_RADIUS = 5.f;
    const ImVec2 NODE_WINDOW_PADDING(8.0f, 8.0f);

    // Create our child canvas
    ImGui::Text("Hold middle mouse button to scroll (%.2f,%.2f)", scrolling.x, scrolling.y);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1,1));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
    ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, (ImVec4)ImColor(60,60,70,200));
    ImGui::BeginChild("scrolling_region", ImVec2(0,0), true, ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoMove);
    ImGui::PushItemWidth(120.0f);

    ImVec2 offset = ImGui::GetCursorScreenPos() - scrolling;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->ChannelsSplit(2);

    // Display grid
    {
        ImVec2 offset = ImGui::GetCursorPos() - scrolling;
        ImU32 GRID_COLOR = ImColor(200,200,200,40);
        float GRID_SZ = 64.0f;
        ImVec2 win_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_sz = ImGui::GetWindowSize();
        for (float x = fmodf(offset.x,GRID_SZ); x < canvas_sz.x; x += GRID_SZ)
            draw_list->AddLine(ImVec2(x,0.0f)+win_pos, ImVec2(x,canvas_sz.y)+win_pos, GRID_COLOR);
        for (float y = fmodf(offset.y,GRID_SZ); y < canvas_sz.y; y += GRID_SZ)
            draw_list->AddLine(ImVec2(0.0f,y)+win_pos, ImVec2(canvas_sz.x,y)+win_pos, GRID_COLOR);
    }

    ImVec2 circle_offset = ImVec2(NODE_SLOT_RADIUS / 2 + 4, 0);

    // Display links
    draw_list->ChannelsSetCurrent(0); // Background
    for (int link_idx = 0; link_idx < links->Size; link_idx++)
    {
        link_t* link     = &(links->Data[link_idx]);
        node_t* node_inp = &(nodes->Data[link->InputIdx]);
        node_t* node_out = &(nodes->Data[link->OutputIdx]);
        ImVec2 p1 = offset + node_inp->GetOutputSlotPos(link->InputSlot) + circle_offset;
        ImVec2 p2 = offset + node_out->GetInputSlotPos(link->OutputSlot) - circle_offset;
        draw_list->AddBezierCurve(p1, p1+ImVec2(+50,0), p2+ImVec2(-50,0), p2, ImColor(200,200,100), 3.0f);
    }

    // Display nodes
    for (int node_idx = 0; node_idx < nodes->Size; node_idx++)
    {
        node_t* node = &(nodes->Data[node_idx]);
        ImGui::PushID(node->ID);
        ImVec2 node_rect_min = offset + node->Pos;

        // Display node contents first
        draw_list->ChannelsSetCurrent(1); // Foreground
        bool old_any_active = ImGui::IsAnyItemActive();
        ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING);
        ImGui::BeginGroup(); // Lock horizontal position
        ImGui::Text("%s", node->Name);
        ImGui::SliderFloat("##value", &node->Value, 0.0f, 1.0f, "Alpha %.2f");
        ImGui::ColorEdit3("##color", &node->Color.x);
        ImGui::EndGroup();

        // Save the size of what we have emitted and whether any of the widgets are being used
        bool node_widgets_active = (!old_any_active && ImGui::IsAnyItemActive());
        node->Size = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING + NODE_WINDOW_PADDING;
        ImVec2 node_rect_max = node_rect_min + node->Size;

        // Display node box
        draw_list->ChannelsSetCurrent(0); // Background
        ImGui::SetCursorScreenPos(node_rect_min);
        ImGui::InvisibleButton("node", node->Size);

        ImGuiIO &io = ImGui::GetIO();
        if (ImGui::IsItemHovered())
        {
            node_hovered_in_scene = node->ID;
            open_context_menu |= ImGui::IsMouseClicked(1);
        }

        bool node_moving_active = ImGui::IsItemActive();
        if (node_widgets_active || node_moving_active) graph->active_node = node->ID;
        if (node_moving_active && ImGui::IsMouseDragging(0)) node->Pos = node->Pos + ImGui::GetIO().MouseDelta;

        ImU32 node_bg_color = (node_hovered_in_list == node->ID || node_hovered_in_scene == node->ID || (node_hovered_in_list == -1 && graph->active_node == node->ID)) ? ImColor(75,75,75) : ImColor(60,60,60);
        draw_list->AddRectFilled(node_rect_min, node_rect_max, node_bg_color, 4.0f);
        draw_list->AddRect(node_rect_min, node_rect_max, ImColor(100,100,100), 4.0f);

        ImVec2 m = io.MousePos;                         // Mouse coords
        float  r = NODE_SLOT_RADIUS * NODE_SLOT_RADIUS; // Radius sqyared

        for (int slot_idx = 0; slot_idx < node->InputsCount; slot_idx++) {
            ImVec2 p = offset + node->GetInputSlotPos(slot_idx) - circle_offset;
            ImVec2 d = p - m;
            if (ImGui::IsMouseReleased(0) && io.KeyCtrl) {
                float  l = d.x * d.x + d.y * d.y;
                if (l < r)  {
                    link->OutputIdx  = node_idx;
                    link->OutputSlot = slot_idx;
                }
            }
            draw_list->AddCircleFilled(p, NODE_SLOT_RADIUS, ImColor(150,150,150,150));
        }
        for (int slot_idx = 0; slot_idx < node->OutputsCount; slot_idx++) {
            ImVec2 p = offset + node->GetOutputSlotPos(slot_idx) + circle_offset;
            ImVec2 d = p - m;
            if (ImGui::IsMouseClicked(0) && io.KeyCtrl) {
                float  l = d.x * d.x + d.y * d.y;
                if (l < r)  {
                    link->InputIdx  = node_idx;
                    link->InputSlot = slot_idx;
                }
            }
            draw_list->AddCircleFilled(p, NODE_SLOT_RADIUS, ImColor(150,150,150,150));
        }

        if (link->InputIdx > -1) {
            node_t* node_inp = &(nodes->Data[link->InputIdx]);
            node_t* node_out = &(nodes->Data[link->OutputIdx]);
            ImVec2 p2 = offset + node_inp->GetOutputSlotPos(link->InputSlot);

            ImVec2 p1;
            if (link->OutputIdx > -1) {
                p1 = offset + node_out->GetInputSlotPos(link->OutputSlot);
            }
            else {
                p1 = m;
                draw_list->AddLine(p1 - circle_offset, p2 + circle_offset, ImColor(250,140,100), 2.0f);
            }
        }

        if (!(io.KeyCtrl)) link->reset();

        ImGui::PopID();
    }
    draw_list->ChannelsMerge();

    // Open context menu
    if (!ImGui::IsAnyItemHovered() && ImGui::IsMouseHoveringWindow() && ImGui::IsMouseClicked(1))
    {
        graph->active_node = node_hovered_in_list = node_hovered_in_scene = -1;
        open_context_menu = true;
    }
    if (open_context_menu)
    {
        ImGui::OpenPopup("context_menu");
        if (node_hovered_in_list  != -1) graph->active_node = node_hovered_in_list;
        if (node_hovered_in_scene != -1) graph->active_node = node_hovered_in_scene;
    }

    // Draw context menu
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8,8));
    if (ImGui::BeginPopup("context_menu"))
    {
        node_t* node = graph->active_node != -1 ? &(nodes->Data[graph->active_node]) : NULL;
        ImVec2 scene_pos = ImGui::GetMousePosOnOpeningCurrentPopup() - offset;
        if (node)
        {
            ImGui::Text("node_t '%s'", node->Name);
            ImGui::Separator();
            if (ImGui::MenuItem("Rename..", NULL, false, false)) {}
            if (ImGui::MenuItem("Delete"  , NULL, false, false)) {}
            if (ImGui::MenuItem("Copy"    , NULL, false, false)) {}
        }
        else
        {
            if (ImGui::MenuItem("Add")) {
             nodes->push_back(node_t(nodes->Size, "New node", scene_pos, 0.5f, ImColor(100,100,200), 2, 5));
            }
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();

    // Scrolling
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(2, 0.0f))
        scrolling = scrolling - ImGui::GetIO().MouseDelta;

    ImGui::PopItemWidth();
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    ImGui::EndGroup();

}
