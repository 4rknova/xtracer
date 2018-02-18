#include <cmath>
#include <xtcore/memutil.tml>

#include <xtcore/cam_erp.h>
#include <xtcore/cam_ods.h>
#include <xtcore/cam_cubemap.h>
#include <xtcore/cam_perspective.h>
#include <xtcore/mat_lambert.h>
#include <xtcore/mat_blinnphong.h>
#include <xtcore/mat_phong.h>
#include <xtcore/sampler_col.h>
#include <xtcore/sampler_tex.h>

#include "graph.h"


node_t::node_t()
    : id(INVALID_ID)
    , inputs(0)
    , outputs(0)
{}

node_t::~node_t()
{}

void node_cam_t::draw_properties()
{
    int spec = 0;
         if (dynamic_cast<CamODS*>        (data)) spec = 1;
    else if (dynamic_cast<CamERP*>        (data)) spec = 2;
    else if (dynamic_cast<CamPerspective*>(data)) spec = 3;
    else if (dynamic_cast<CamCubemap*>    (data)) spec = 4;

    ImGui::Text("Camera.%s", xtcore::pool::str::get(id));

    switch (spec) {
        case 1: {
            ImGui::Text("Type : ODS");
            CamODS *p = (CamODS*)data;
            textedit_float3("position"   , p->position   , 0.1);
            textedit_float3("orientation", p->orientation, 0.1);
            textedit_float ("IPD"        , p->ipd        , 0.1);
            break;
        }
        case 2: {
            ImGui::Text("Type : ERP");
            CamERP *p = (CamERP*)data;
            textedit_float3("position"   , p->position   , 0.1);
            textedit_float3("orientation", p->orientation, 0.1);
            break;
        }
        case 3: {
            ImGui::Text("Type : Perspective");
            CamPerspective *p = (CamPerspective*)data;
            textedit_float3("position"   , p->position   , 0.1);
            textedit_float3("target"     , p->target     , 0.1);
            textedit_float3("up"         , p->up         , 0.1);
            textedit_float ("fov"        , p->fov        , 0.1);
            textedit_float ("aperture"   , p->aperture   , 0.1);
            textedit_float ("flength"    , p->flength    , 0.1);
            break;
        }
        case 4: {
            ImGui::Text("Type : Cubemap");
            CamCubemap *p = (CamCubemap*)data;
            textedit_float3("position"   , p->position   , 0.1);
            break;
        }
    }
}

void node_obj_t::draw_properties()
{
    ImGui::TextColored(ImVec4(.1,1.,.9,1.), "%s", xtcore::pool::str::get(id));
    ImGui::Text("Geometry: %s", xtcore::pool::str::get(((xtcore::assets::Object*)data)->geometry));
    ImGui::Text("Material: %s", xtcore::pool::str::get(((xtcore::assets::Object*)data)->material));
}

void node_mat_t::draw_properties()
{
    int spec = 0;
         if (dynamic_cast<xtcore::assets::MaterialLambert*>   (data)) spec = 1;
    else if (dynamic_cast<xtcore::assets::MaterialBlinnPhong*>(data)) spec = 2;
    else if (dynamic_cast<xtcore::assets::MaterialPhong*>     (data)) spec = 3;

    ImGui::Text("Material.%s", xtcore::pool::str::get(id));

    switch (spec) {
        case 1: {
            ImGui::Text("Shader: Lambert");
            break;
        }
        case 2: {
            ImGui::Text("Shader: BlinnPhong");
            break;
        }
        case 3: {
            ImGui::Text("Shader: Phong");
            break;
        }
    }

    int sz_scalar  = data->get_scalar_count();
    int sz_sampler = data->get_sampler_count();

    for (int i = 0; i < sz_scalar; ++i) {
        std::string name;
        float &f = data->get_scalar_by_index(i, &name);
        textedit_float(name.c_str(), f, 0.1f);
    }

    for (int i = 0; i < sz_sampler; ++i) {
        std::string name;
        xtcore::assets::ISampler *s = data->get_sampler_by_index(i, &name);

        if (dynamic_cast<xtcore::assets::SolidColor*>(s)) {
            nimg::ColorRGBf col;
            ((xtcore::assets::SolidColor*)(s))->get(col);
            float c[4] = {col.r(), col.g(), col.b(), 1.};
            ImGui::ColorPicker4(name.c_str(), c, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
            col.r(c[0]);
            col.g(c[1]);
            col.b(c[2]);
            ((xtcore::assets::SolidColor*)(s))->set(col);
        }
        else if (dynamic_cast<xtcore::assets::Texture2D*>(s)) {
            ImGui::Text("Texture: %s", name.c_str());
        }
    }
}

void node_geo_t::draw_properties()
{
    ImGui::Text("Geometry.%s", xtcore::pool::str::get(id));
}

ImVec2 node_t::get_input_slot_position(int slot_no) const
{
    ImVec2 sz = get_size();
    return ImVec2(position.x, position.y + sz.y * ((float)slot_no+1) / ((float)inputs+1));
}

ImVec2 node_t::get_output_slot_position(int slot_no) const
{
    ImVec2 sz = get_size();
    return ImVec2(position.x + sz.x - 1, position.y + sz.y * ((float)slot_no+1) / ((float)outputs+1));
}

ImVec2 node_t::get_size() const
{
    ImVec2 p = NODE_WINDOW_PADDING;
    return ImGui::GetItemRectSize() + p + p;
}

link_t::link_t(int input_idx, int input_slot, int output_idx, int output_slot)
{
    InputIdx   = input_idx;
    InputSlot  = input_slot;
    OutputIdx  = output_idx;
    OutputSlot = output_slot;
}

link_t::link_t()
    : InputIdx  (INVALID_ID)
    , OutputIdx (INVALID_ID)
    , InputSlot (INVALID_ID)
    , OutputSlot(INVALID_ID)
{}

graph_t::graph_t()
    : active_node(INVALID_ID)
{}

graph_t::~graph_t()
{
    auto net = nodes.end();
    auto let = links.end();

    for (auto it = nodes.begin(); it != net; ++it) xtcore::memory::safe_delete<node_t>(*it);
    for (auto it = links.begin(); it != let; ++it) xtcore::memory::safe_delete<link_t>(*it);
    nodes.clear();
    links.clear();
}

void draw_graph(graph_t *graph, link_t *link)
{
    ImVector<node_t*> *nodes = &(graph->nodes);
    ImVector<link_t*> *links = &(graph->links);

    static ImVec2 scrolling = ImVec2(0.0f, 0.0f);
    static bool show_grid = true;

    // Draw a list of nodes on the left side
    bool open_context_menu = false;
    int node_hovered_in_list = -1;
    int node_hovered_in_scene = -1;

    ImGui::SameLine();
    ImGui::BeginGroup();

    // Create our child canvas
    ImGui::Text("Hold middle mouse button to scroll (%.2f,%.2f)", scrolling.x, scrolling.y);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1,1));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
    ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, (ImVec4)ImColor(60,60,70,200));
    ImGui::BeginChild("scrolling_region", ImVec2(0,0), true, ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse|ImGuiWindowFlags_NoMove);
    ImGui::PushItemWidth(100.0f);

    ImVec2 offset = ImGui::GetCursorScreenPos() - scrolling;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->ChannelsSplit(2);

    // Display grid
    {
        ImVec2 offset = ImGui::GetCursorPos() - scrolling;
        ImU32 GRID_COLOR = ImColor(200,200,200,40);
        float GRID_SZ = 32.0f;
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
    for (int link_idx = 0; link_idx < links->size(); link_idx++)
    {
        link_t* link     = links->Data[link_idx];
        node_t* node_inp = nodes->Data[link->InputIdx];
        node_t* node_out = nodes->Data[link->OutputIdx];
        ImVec2 p1 = offset + node_inp->get_output_slot_position(link->InputSlot) + circle_offset;
        ImVec2 p2 = offset + node_out->get_input_slot_position(link->OutputSlot) - circle_offset;
        draw_list->AddBezierCurve(p1, p1+ImVec2(+50,0), p2+ImVec2(-50,0), p2, ImColor(200,200,100), 3.0f);
    }

    // Display nodes
    for (auto it = nodes->begin(); it != nodes->end(); ++it)
    {
        (*it)->draw(draw_list, offset, circle_offset, node_hovered_in_list, node_hovered_in_scene);
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
        node_t* node = graph->active_node != -1 ? nodes->Data[graph->active_node] : NULL;
        ImVec2 scene_pos = ImGui::GetMousePosOnOpeningCurrentPopup() - offset;
        if (node)
        {
            ImGui::Text("node_t '%s'", xtcore::pool::str::get(node->id));
            ImGui::Separator();
            if (ImGui::MenuItem("Rename..", NULL, false, false)) {}
            if (ImGui::MenuItem("Delete"  , NULL, false, false)) {}
            if (ImGui::MenuItem("Copy"    , NULL, false, false)) {}
        }
        else
        {
//            if (ImGui::MenuItem("Add")) {
//             nodes->push_back(node_t(nodes->size, "New node", scene_pos, 0.5f, ImColor(100,100,200), 2, 5));
//            }
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
