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
#include <nmath/plane.h>
#include <nmath/triangle.h>
#include <nmath/sphere.h>
#include <nmesh/mesh.h>

#include "graph.h"

#define COL_NODE_OUTLINE ImColor(100,100,100)
#define COL_NODE_NORMAL  ImColor( 60, 60, 60)
#define COL_NODE_HOVERED ImColor( 75, 75, 75)
#define COL_LINK_NORMAL  ImColor(175,175, 75)
#define COL_LINK_HOVERED ImColor(255, 25, 40)

namespace gui {
    namespace graph {

node_t::node_t()
    : id(INVALID_ID)
    , name(0)
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

    ImGui::Text("Camera.%s", xtcore::pool::str::get(name));

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
    ImGui::TextColored(ImVec4(.1,1.,.9,1.), "%s", xtcore::pool::str::get(name));
    ImGui::Text("Geometry: %s", xtcore::pool::str::get(((xtcore::assets::Object*)data)->geometry));
    ImGui::Text("Material: %s", xtcore::pool::str::get(((xtcore::assets::Object*)data)->material));
}

void node_mat_t::draw_properties()
{
    int spec = 0;
         if (dynamic_cast<xtcore::assets::MaterialLambert*>   (data)) spec = 1;
    else if (dynamic_cast<xtcore::assets::MaterialBlinnPhong*>(data)) spec = 2;
    else if (dynamic_cast<xtcore::assets::MaterialPhong*>     (data)) spec = 3;

    ImGui::Text("Material.%s", xtcore::pool::str::get(name));

    switch (spec) {
        case 1: { ImGui::Text("Shader: Lambert");    break; }
        case 2: { ImGui::Text("Shader: BlinnPhong"); break; }
        case 3: { ImGui::Text("Shader: Phong");      break; }
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
    int spec = 0;
         if (dynamic_cast<NMath::Plane*>    (data)) spec = 1;
    else if (dynamic_cast<NMath::Triangle*> (data)) spec = 2;
    else if (dynamic_cast<NMath::Sphere*>   (data)) spec = 3;
    else if (dynamic_cast<nmesh::Mesh*>     (data)) spec = 4;

    ImGui::Text("Geometry.%s", xtcore::pool::str::get(name));

    switch (spec) {
        case 1: {
            NMath::Plane *p = (NMath::Plane*)data;
            textedit_float3("normal"  , p->normal   , 0.1);
            textedit_double("distance", p->distance , 0.1);
            break;
        }
        case 2: {
            NMath::Triangle *p = (NMath::Triangle*)data;
            textedit_float3("v0" , p->v[0]   , 0.1);
            textedit_float3("v1" , p->v[1]   , 0.1);
            textedit_float3("v2" , p->v[2]   , 0.1);
            textedit_float3("n0" , p->n[0]   , 0.1);
            textedit_float3("n1" , p->n[1]   , 0.1);
            textedit_float3("n2" , p->n[2]   , 0.1);
            textedit_float2("tc0", p->tc[0]  , 0.1);
            textedit_float2("tc1", p->tc[1]  , 0.1);
            textedit_float2("tc2", p->tc[2]  , 0.1);
            break;
        }
        case 3: {
            NMath::Sphere *p = (NMath::Sphere*)data;
            textedit_float3("origin", p->origin, 0.1);
            textedit_double("radius", p->radius, 0.1);
            break;
        }
        case 4: {
            nmesh::Mesh *p = (nmesh::Mesh*)data;
            break;
        }
    }
}

ImVec2 node_t::get_input_slot_position(int slot_no) const
{
    float seg = size.y / inputs;
    return position + ImVec2(-NODE_SLOT_RADIUS, seg * (slot_no + 0.5f));
}

ImVec2 node_t::get_output_slot_position(int slot_no) const
{
    float seg = size.y / outputs;
    return position + ImVec2(NODE_SLOT_RADIUS + size.x, seg * (slot_no + 0.5f));
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
    clear();
}

void graph_t::clear()
{
    auto net = nodes.end();
    auto let = links.end();

    for (auto it = nodes.begin(); it != net; ++it) xtcore::memory::safe_delete<node_t>(*it);
    for (auto it = links.begin(); it != let; ++it) xtcore::memory::safe_delete<link_t>(*it);
    nodes.clear();
    links.clear();
}

bool node_t::draw(ImDrawList *draw_list, ImVec2 offset, ImVec2 circle_offset, node_t *selected)
{
    ImGui::PushID(id);
    ImVec2 node_rect_min = offset + position;

    // Display node contents first
    draw_list->ChannelsSetCurrent(2); // Foreground
    bool old_any_active = ImGui::IsAnyItemActive();
    ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING);
    ImGui::BeginGroup();
    //ImGui::Text("%s", xtcore::pool::str::get(name));
    draw_properties();
    ImGui::EndGroup();

    // Save the size of what we have emitted and whether any of the widgets are being used
    bool node_widgets_active = (!old_any_active && ImGui::IsAnyItemActive());
    size = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING + NODE_WINDOW_PADDING;
    ImVec2 node_rect_max = node_rect_min + size;

    // Display node box
    draw_list->ChannelsSetCurrent(1); // Background
    ImGui::SetCursorScreenPos(node_rect_min);
    ImGui::InvisibleButton("node", size);

    ImGuiIO &io = ImGui::GetIO();

    bool node_hovered = ImGui::IsItemHovered();

    ImU32 node_bg_color = node_hovered ? COL_NODE_NORMAL : COL_NODE_HOVERED;

    bool node_moving_active = ImGui::IsItemActive();
    if (node_moving_active) {
        selected = this;
        if (ImGui::IsMouseDragging(0)) {
            position = position + ImGui::GetIO().MouseDelta;
        }
    }

    draw_list->AddRectFilled(node_rect_min, node_rect_max, node_bg_color, 4.0f);
    draw_list->AddRect(node_rect_min, node_rect_max, COL_NODE_OUTLINE, 4.0f);

    for (int slot_idx = 0; slot_idx < inputs; ++slot_idx) {
        draw_list->AddCircleFilled(offset + get_input_slot_position(slot_idx)
                                 , NODE_SLOT_RADIUS, ImColor(150,150,150,150));
    }
    for (int slot_idx = 0; slot_idx < outputs; ++slot_idx) {
        draw_list->AddCircleFilled(offset + get_output_slot_position(slot_idx)
                                 , NODE_SLOT_RADIUS, ImColor(150,150,150,150));
    }

    ImGui::PopID();

    return node_hovered;
}

void draw(graph_t *graph, const xtcore::Scene *scene)
{
    ImVector<node_t*> *nodes = &(graph->nodes);
    ImVector<link_t*> *links = &(graph->links);

    static ImVec2 scrolling = ImVec2(0.0f, 0.0f);
    static bool show_grid = true;

    // Draw a list of nodes on the left side
    bool open_context_menu = false;

    ImGui::SameLine();
    ImGui::BeginGroup();

    // Create our child canvas
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1,1));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
    ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, (ImVec4)ImColor(60,60,70,200));
    ImGui::BeginChild("scrolling_region", ImVec2(0,0), true, ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse|ImGuiWindowFlags_NoMove);
    ImGui::PushItemWidth(100.0f);

    ImVec2 offset = ImGui::GetCursorScreenPos() - scrolling;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->ChannelsSplit(3);

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

    ImVec2 circle_offset = ImVec2(NODE_SLOT_RADIUS / 2, 0);

    // Display nodes
    int hovered_id = INVALID_ID;

    static node_t *selected = 0;
    for (auto it = nodes->begin(); it != nodes->end(); ++it) {
        bool hovered = (*it)->draw(draw_list, offset, circle_offset, selected);
        if (hovered) hovered_id = (*it)->id;
    }

    // Display links
    draw_list->ChannelsSetCurrent(0); // Background

    for (int link_idx = 0; link_idx < links->size(); link_idx++)
    {
        link_t* link     = links->Data[link_idx];
        node_t* node_inp = nodes->Data[link->InputIdx];
        node_t* node_out = nodes->Data[link->OutputIdx];
        ImVec2 p1 = offset + node_out->get_input_slot_position(link->OutputSlot);
        ImVec2 p2 = offset + node_inp->get_output_slot_position(link->InputSlot);

        bool is_linked_item_hovered = (node_inp->id == hovered_id) || (node_out->id == hovered_id);

        ImColor col = is_linked_item_hovered ? COL_LINK_HOVERED : COL_LINK_NORMAL;

        draw_list->AddBezierCurve(p1, p1+ImVec2(-50,0), p2+ImVec2(50,0), p2, col, 3.0f);
    }
    draw_list->ChannelsMerge();

    if (!ImGui::IsAnyItemHovered() && ImGui::IsMouseHoveringWindow() && ImGui::IsMouseClicked(1))
    {
        open_context_menu = true;
    }
    if (open_context_menu)
    {
        ImGui::OpenPopup("context_menu");
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
            if (ImGui::MenuItem("Add Camera")) {
               build(graph, scene);
            }
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();

    // Scrolling
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(2, 0.0f))
        scrolling = scrolling + ImGui::GetIO().MouseDelta;

    ImGui::PopItemWidth();
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    ImGui::EndGroup();

    ImGui::SetCursorScreenPos(ImVec2(44, 50) + NODE_WINDOW_PADDING);
    ImGui::BeginGroup();
    if (selected) {
        printf("ok\n");
        selected->draw_properties();
    }
    ImGui::EndGroup();

}

void build(graph_t *graph, const xtcore::Scene *scene)
{
    int id = 0;

    if (!graph || !scene) return;

    graph->clear();


    std::map<HASH_UINT64, int> mat, geo;

    int x = 0, y = 0;
    // Cameras
    {
        const float interval = 300.f;
        auto et = scene->m_cameras.end();
        y = -interval * scene->m_cameras.size() / 2;
        for (auto it = scene->m_cameras.begin(); it != et; ++it) {
            node_cam_t *node = new node_cam_t;
            node->inputs  = 0;
            node->outputs = 0;
            node->id   = id++;
            node->name = (*it).first;
            node->data = (*it).second;
            node->position = ImVec2(x, y);
            graph->nodes.push_back(node);
            y += interval;
        }
    }
    x = 1000;
    // Materials
    {
        const float interval = 400.f;
        auto et = scene->m_materials.end();
        y = -interval * scene->m_materials.size() / 2;
        for (auto it = scene->m_materials.begin(); it != et; ++it) {
            node_mat_t *node = new node_mat_t;
            node->inputs  = 1;
            node->outputs = 0;
            node->id   = id++;
            node->name = (*it).first;
            node->data = (*it).second;
            node->position = ImVec2(x, y);
            graph->nodes.push_back(node);
            mat[node->name] = node->id;
            y += interval;
        }
    }
    x = 750;
    // Geometry
    {
        const float interval = 200.f;
        auto et = scene->m_geometry.end();
        y = -interval * scene->m_geometry.size() / 2;
        for (auto it = scene->m_geometry.begin(); it != et; ++it) {
            node_geo_t *node = new node_geo_t;
            node->inputs  = 1;
            node->outputs = 0;
            node->id   = id++;
            node->name = (*it).first;
            node->data = (*it).second;
            node->position = ImVec2(x, y);
            graph->nodes.push_back(node);
            geo[node->name] = node->id;
            y += interval;
        }
    }
    x = 500;
    // Objects
    {
        const float interval = 100.f;
        auto et = scene->m_objects.end();
        y = -interval * scene->m_objects.size() / 2;
        for (auto it = scene->m_objects.begin(); it != et; ++it) {
            node_obj_t *node = new node_obj_t;
            node->inputs  = 0;
            node->outputs = 2;
            node->id   = id++;
            node->name = (*it).first;
            node->data = (*it).second;
            node->position = ImVec2(x, y);
            graph->nodes.push_back(node);

            link_t *link0 = new link_t;
            link0->InputIdx  = node->id;
            link0->OutputIdx = geo[node->data->geometry];
            link0->InputSlot = 0;
            link0->OutputSlot = 0;
            graph->links.push_back(link0);

            link_t *link1 = new link_t;
            link1->InputIdx  = node->id;
            link1->OutputIdx = mat[node->data->material];
            link1->InputSlot = 1;
            link1->OutputSlot = 0;
            graph->links.push_back(link1);

            y += interval;
        }
    }
}

    } /* namespace graph */
} /* namespace gui */