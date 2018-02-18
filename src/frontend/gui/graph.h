#ifndef SINGULARITY_GRAPH_H_INCLUDED
#define SINGULARITY_GRAPH_H_INCLUDED

#include <vector>
#include <xtcore/strpool.h>

#include <xtcore/camera.h>
#include <xtcore/object.h>
#include <xtcore/geometry.h>
#include <xtcore/material.h>

#include "imgui_extra.h"

#define INVALID_ID          ( -1)
#define NODE_SLOT_RADIUS    (5.f)
#define NODE_WINDOW_PADDING ImVec2(8.f,8.f)

struct node_t
{
    HASH_UINT64 id;
    ImVec2      position;
    size_t      inputs;
    size_t      outputs;

             node_t();
    virtual ~node_t();

    ImVec2 get_input_slot_position(int slot_no) const;
    ImVec2 get_output_slot_position(int slot_no) const;
    ImVec2 get_size() const;

    virtual void draw_properties() = 0;

    void draw(ImDrawList *draw_list, ImVec2 offset, ImVec2 circle_offset, int node_hovered_in_list, int node_hovered_in_scene)
    {
        ImGui::PushID(id);
        ImVec2 node_rect_min = offset + position;

        // Display node contents first
        draw_list->ChannelsSetCurrent(1); // Foreground
        bool old_any_active = ImGui::IsAnyItemActive();
        ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING);
        ImGui::BeginGroup();
        draw_properties();
        ImGui::EndGroup();

        // Save the size of what we have emitted and whether any of the widgets are being used
        bool node_widgets_active = (!old_any_active && ImGui::IsAnyItemActive());
        ImVec2 sz = get_size();
        ImVec2 node_rect_max = node_rect_min + sz;

        // Display node box
        draw_list->ChannelsSetCurrent(0); // Background
        ImGui::SetCursorScreenPos(node_rect_min);
        ImGui::InvisibleButton("node", sz);

        ImGuiIO &io = ImGui::GetIO();
/*       if (ImGui::IsItemHovered())
        {
            node_hovered_in_scene = id;
            open_context_menu |= ImGui::IsMouseClicked(1);
        }
*/
        bool node_moving_active = ImGui::IsItemActive();
//        if (node_widgets_active || node_moving_active) graph->active_node = id;
        if (node_moving_active && ImGui::IsMouseDragging(0)) position = position + ImGui::GetIO().MouseDelta;

        ImU32 node_bg_color = ImColor(60,60,60);
//        ImU32 node_bg_color = (node_hovered_in_list == id || node_hovered_in_scene == id || (node_hovered_in_list == -1 && graph->active_node == id)) ? ImColor(75,75,75) : ImColor(60,60,60);
        draw_list->AddRectFilled(node_rect_min, node_rect_max, node_bg_color, 4.0f);
        draw_list->AddRect(node_rect_min, node_rect_max, ImColor(100,100,100), 4.0f);

        ImVec2 m = io.MousePos;                         // Mouse coords
        float  r = NODE_SLOT_RADIUS * NODE_SLOT_RADIUS; // Radius sqyared

        for (int slot_idx = 0; slot_idx < inputs; slot_idx++) {
            ImVec2 p = offset + get_input_slot_position(slot_idx) - circle_offset;
            ImVec2 d = p - m;
            draw_list->AddCircleFilled(p, NODE_SLOT_RADIUS, ImColor(150,150,150,150));
        }
        for (int slot_idx = 0; slot_idx < outputs; slot_idx++) {
            ImVec2 p = offset + get_output_slot_position(slot_idx) - circle_offset;
            ImVec2 d = p - m;
            draw_list->AddCircleFilled(p, NODE_SLOT_RADIUS, ImColor(150,150,150,150));
        }


        ImGui::PopID();
    }
};

struct node_cam_t : public node_t
{
    xtcore::assets::ICamera *data;
    virtual void draw_properties();
};

struct node_obj_t : public node_t
{
    xtcore::assets::Object *data;
    virtual void draw_properties();
};

struct node_mat_t : public node_t
{
    xtcore::assets::IMaterial *data;
    virtual void draw_properties();
};

struct node_geo_t : public node_t
{
    xtcore::assets::Geometry *data;
    virtual void draw_properties();
};

struct link_t
{
    int InputIdx, InputSlot, OutputIdx, OutputSlot;

    link_t();
    link_t(int input_idx, int input_slot, int output_idx, int output_slot);
};

struct graph_t {
    ImVector<node_t*> nodes;
    ImVector<link_t*> links;

    int active_node;

     graph_t();
    ~graph_t();
};

void draw_nlist(graph_t *graph);
void draw_graph(graph_t *graph, link_t *link);

#endif /* SINGULARITY_GRAPH_H_INCLUDED */
