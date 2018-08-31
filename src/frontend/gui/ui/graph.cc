#include <cmath>
#include <xtcore/memutil.tml>

#include <xtcore/math/plane.h>
#include <xtcore/math/triangle.h>
#include <xtcore/math/sphere.h>
#include <xtcore/camera/erp.h>
#include <xtcore/camera/ods.h>
#include <xtcore/camera/cubemap.h>
#include <xtcore/camera/perspective.h>
#include <xtcore/material/lambert.h>
#include <xtcore/material/blinnphong.h>
#include <xtcore/material/phong.h>
#include <xtcore/material/emissive.h>
#include <xtcore/sampler_col.h>
#include <xtcore/sampler_tex.h>
#include <xtcore/mesh.h>
#include "graphics.h"
#include "graph.h"
#include "../util.h"

#define COL_LINE_NORMAL   (ImColor(0.00f, 0.00f, 0.00f, 1.0f))
#define COL_LINE_HOVERED  (ImColor(1.00f, 1.00f, 1.00f, 0.5f))
#define COL_NODE_NORMAL   (ImColor(0.40f, 0.40f, 0.40f, 1.0f))
#define COL_NODE_HOVERED  (ImColor(0.40f, 0.40f, 0.40f, 0.5f))
#define COL_SLOT_NORMAL   (ImColor(0.40f, 0.75f, 1.00f, 0.5f))
#define COL_SLOT_HOVERED  (ImColor(0.00f, 0.00f, 0.00f, 0.8f))
#define COL_LINK_NORMAL   (ImColor(175,175, 75,255))
#define COL_LINK_HOVERED  (ImColor(255, 25, 40,255))
#define COL_GRID          (ImColor(200,200,200, 20))
#define COL_BG            (ImColor(0.00f, 0.00f, 0.00f, 0.1f))
#define FLT_GRID_BLOCK    (16.0f)
#define VEC_NODE_SIZE     (ImVec2(200,250))
#define STR_EMPTY         ("")
#define INVALID_ID        ( -1)
#define NODE_SLOT_RADIUS  (6.f)
#define LAYER_FOREGROUND  (2)
#define LAYER_BACKGROUND  (1)
#define LAYER_BOTTOM      (0)

#define NODE_LABEL_BLANK  ("UNKNOWN")
#define NODE_LABEL_CAM    ("Camera")
#define NODE_LABEL_OBJ    ("Object")
#define NODE_LABEL_MAT    ("Material")
#define NODE_LABEL_GEO    ("Surface")
#define NODE_LABEL_COL    ("Sampler")

#define MOUSE_BUTTON_LEFT   (0)
#define MOUSE_BUTTON_RIGHT  (1)
#define MOUSE_BUTTON_MIDDLE (2)

namespace gui {
    namespace graph {

node_t::node_t()
    : id(INVALID_ID)
    , name(0)
    , size(VEC_NODE_SIZE)
    , inputs(0)
    , outputs(0)
    , label(0)
    , is_open(false)
{}

node_t::~node_t()
{}

template<>
void node_cam_t::init()
{
    label        = NODE_LABEL_CAM;
    size         = ImVec2(230, 350);
    color_header = ImColor(90,135,200,255);
}

template<>
void node_cam_t::draw_properties()
{
    int spec = 0;
         if (dynamic_cast<xtcore::camera::ODS*>        (data)) spec = 1;
    else if (dynamic_cast<xtcore::camera::ERP*>        (data)) spec = 2;
    else if (dynamic_cast<xtcore::camera::Perspective*>(data)) spec = 3;
    else if (dynamic_cast<xtcore::camera::Cubemap*>    (data)) spec = 4;

    switch (spec) {
        case 1: {
            ImGui::Text("Type : ODS");
            xtcore::camera::ODS *p = (xtcore::camera::ODS*)data;
            textedit_float3("position"   , p->position   , 0.1);
            textedit_float3("orientation", p->orientation, 0.1);
            textedit_float ("IPD"        , p->ipd        , 0.1);
            break;
        }
        case 2: {
            ImGui::Text("Type : ERP");
            xtcore::camera::ERP *p = (xtcore::camera::ERP*)data;
            textedit_float3("position"   , p->position   , 0.1);
            textedit_float3("orientation", p->orientation, 0.1);
            break;
        }
        case 3: {
            ImGui::Text("Type : Perspective");
            xtcore::camera::Perspective *p = (xtcore::camera::Perspective*)data;
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
            xtcore::camera::Cubemap *p = (xtcore::camera::Cubemap*)data;
            textedit_float3("position"   , p->position   , 0.1);
            break;
        }
    }
}

template<>
void node_obj_t::init()
{
    label        = NODE_LABEL_OBJ;
    color_header = ImColor(100,55,25,255);
}

template<>
void node_obj_t::draw_properties()
{
    ImGui::Text("Geometry: %s", xtcore::pool::str::get(((xtcore::asset::Object*)data)->geometry));
    ImGui::Text("Material: %s", xtcore::pool::str::get(((xtcore::asset::Object*)data)->material));
}

template<>
void node_mat_t::init()
{
    label        = NODE_LABEL_MAT;
    color_header = ImColor(29,100,25,255);
}

template<>
void node_mat_t::draw_properties()
{
    int spec = 0;
         if (dynamic_cast<xtcore::asset::material::Lambert*>   (data)) spec = 1;
    else if (dynamic_cast<xtcore::asset::material::BlinnPhong*>(data)) spec = 2;
    else if (dynamic_cast<xtcore::asset::material::Phong*>     (data)) spec = 3;
    else if (dynamic_cast<xtcore::asset::material::Emissive*>  (data)) spec = 4;

    switch (spec) {
        case 1: { ImGui::Text("Shader: Lambert");    break; }
        case 2: { ImGui::Text("Shader: BlinnPhong"); break; }
        case 3: { ImGui::Text("Shader: Phong");      break; }
        case 4: { ImGui::Text("Shader: Emissive");   break; }
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
        xtcore::sampler::ISampler *s = data->get_sampler_by_index(i, &name);

        if (dynamic_cast<xtcore::sampler::SolidColor*>(s)) {
            nimg::ColorRGBf col;
            ((xtcore::sampler::SolidColor*)(s))->get(col);
            float c[4] = {col.r(), col.g(), col.b(), 1.};
            ImGui::ColorPicker4(name.c_str(), c, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
            col.r(c[0]);
            col.g(c[1]);
            col.b(c[2]);
            ((xtcore::sampler::SolidColor*)(s))->set(col);
        }
        else if (dynamic_cast<xtcore::sampler::Texture2D*>(s)) {
            ImGui::Text("Texture: %s", name.c_str());
        }
    }
}

template<>
void node_geo_t::init()
{
    label        = NODE_LABEL_GEO;
    color_header = ImColor(39,100,100,255);
}

template<>
void node_geo_t::draw_properties()
{
    int spec = 0;
         if (dynamic_cast<xtcore::surface::Plane*>    (data)) spec = 1;
    else if (dynamic_cast<xtcore::surface::Triangle*> (data)) spec = 2;
    else if (dynamic_cast<xtcore::surface::Sphere*>   (data)) spec = 3;
    else if (dynamic_cast<xtcore::surface::Mesh*>     (data)) spec = 4;

    switch (spec) {
        case 1: {
            xtcore::surface::Plane *p = (xtcore::surface::Plane*)data;
            textedit_float3("normal"  , p->normal, 0.1);
            textedit_double("distance", p->offset, 0.1);
            break;
        }
        case 2: {
            xtcore::surface::Triangle *p = (xtcore::surface::Triangle*)data;
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
            xtcore::surface::Sphere *p = (xtcore::surface::Sphere*)data;
            textedit_float3("origin", p->origin, 0.1);
            textedit_double("radius", p->radius, 0.1);
            break;
        }
        case 4: {
            xtcore::surface::Mesh *p = (xtcore::surface::Mesh*)data;
            break;
        }
    }
}

template<>
void node_col_t::init()
{
    label        = NODE_LABEL_COL;
    color_header = ImColor(59,130,140,255);
}


template<>
void node_col_t::draw_properties()
{
    if (!data) return;

    size = ImVec2(175, 220);
    outputs = 1;

    nimg::ColorRGBf col;
    data->get(col);
    float c[4] = {col.r(), col.g(), col.b(), 1.};
    ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_NoSidePreview;
    ImGui::Dummy(ImVec2(0,10));
    ImGui::Indent(size.x * 0.5/4.0);
    ImGui::ColorPicker3(STR_EMPTY, c, flags);
    col.r(c[0]); col.g(c[1]); col.b(c[2]);
    data->set(col);
}

ImVec2 node_t::get_input_slot_position(int slot_no) const
{
    float seg = size.y / inputs;
    return position + ImVec2(-NODE_SLOT_RADIUS, is_open ? seg * (slot_no + 0.5f) : 10.0f);
}

ImVec2 node_t::get_output_slot_position(int slot_no) const
{
    float seg = size.y / outputs;
    return position + ImVec2(NODE_SLOT_RADIUS + size.x, is_open ? seg * (slot_no + 0.5f) : 10.0f);
}

link_t::link_t(int input_idx, int input_slot, int output_idx, int output_slot)
{
    a_id   = input_idx;
    a_slot  = input_slot;
    b_id  = output_idx;
    b_slot = output_slot;
}

link_t::link_t()
    : a_id  (INVALID_ID)
    , b_id (INVALID_ID)
    , a_slot (INVALID_ID)
    , b_slot(INVALID_ID)
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

bool node_t::draw(ImDrawList *dl, ImVec2 offset, ImVec2 circle_offset)
{
/*
    static float scale = 1.0;

    if (ImGui::GetIO().KeyCtrl)
    {
        scale = MAX(1.0, scale + ImGui::GetIO().MouseWheel / 250.);
        printf("%f\n", scale);
    }

    size = ImVec2(scale * size.x, scale * size.y);
*/

    bool node_hovered = false
        ,node_active  = false;

    int rounded_corners = ImDrawCornerFlags_TopLeft | ImDrawCornerFlags_TopRight;
    ImVec2 p;
    ImColor col_br
          , col_bg
          , col_sl;

    ImGui::PushID(id);
    {
        ImVec2 p_min = offset + position;
        dl->ChannelsSetCurrent(LAYER_FOREGROUND);
        ImVec2 node_rect_max = p_min + size;
        ImGui::SetCursorScreenPos(p_min);
        ImGui::PushItemWidth(size.x);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10,10));
        ImGui::BeginGroup();
        {
            p = ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos(p_min + ImVec2(5,3));
            ImGui::PushItemWidth(size.x * 3.0/4.0);
            if (ImGui::Button(is_open ? "-" : "+", ImVec2(15,15))) is_open = !is_open;
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1,1,1,1), label ? label : NODE_LABEL_BLANK);
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1,1,1,1), ":");
            ImGui::SameLine();
            const char *str_name = xtcore::pool::str::get(name);
            ImGui::TextColored(ImVec4(1,1,1,1), str_name ? str_name : NODE_LABEL_BLANK);
            if (is_open) draw_properties();
            ImGui::PopItemWidth();
        }
        ImGui::EndGroup();
        ImGui::PopStyleVar();
        ImGui::PopItemWidth();

        ImVec2 compacted_size = ImVec2(size.x-1, 20);

        dl->ChannelsSetCurrent(LAYER_BACKGROUND); // Background
        ImGui::SetCursorScreenPos(p_min);
        ImGui::InvisibleButton("node", is_open ? size : compacted_size);
        node_hovered = ImGui::IsItemHovered();
        node_active  = ImGui::IsItemActive();
        col_bg = node_hovered ? COL_NODE_HOVERED : COL_NODE_NORMAL;
        col_sl = node_hovered ? COL_SLOT_HOVERED : COL_SLOT_NORMAL;
        col_br = node_hovered ? COL_LINE_HOVERED : COL_LINE_NORMAL;
        if (is_open) dl->AddRectFilled(p_min, node_rect_max, col_bg, 4.0f);
        dl->AddRectFilled(p + ImVec2(1,1), p + ImVec2(size.x-1, 20), color_header, 4., rounded_corners);
        if (is_open) {
            dl->AddRect(p_min, node_rect_max, col_br, 4.0f);
            dl->AddLine(p + ImVec2(1,20), p + ImVec2(size.x-1, 20), col_br);
        }

        if (node_active && ImGui::IsMouseDragging(MOUSE_BUTTON_LEFT)) {
            position = position + ImGui::GetIO().MouseDelta;
        }

        for (int i = 0; i < inputs; ++i) {
            ImVec2 sp = offset + get_input_slot_position(i);
            dl->AddCircleFilled(sp, NODE_SLOT_RADIUS, col_sl);
            dl->AddCircle      (sp, NODE_SLOT_RADIUS, col_br);
        }
        for (int i = 0; i < outputs; ++i) {
            ImVec2 sp = offset + get_output_slot_position(i);
            dl->AddCircleFilled(sp, NODE_SLOT_RADIUS, col_sl);
            dl->AddCircle      (sp, NODE_SLOT_RADIUS, col_br);
        }
    }
    ImGui::PopID();

    return node_hovered;
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
        const float interval = 30.f;
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
            node->init();
            graph->nodes.push_back(node);
            y += interval;
        }
    }
    x = 900;
    // Materials
    {
        const float interval = 30.f;
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
            node->init();
            graph->nodes.push_back(node);
            mat[node->name] = node->id;
            y += interval;
        }
    }
    x = 600;
    // Geometry
    {
        const float interval = 30.f;
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
            node->init();
            graph->nodes.push_back(node);
            geo[node->name] = node->id;
            y += interval;
        }
    }
    x = 300;
    // Objects
    {
        const float interval = 30.f;
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
            node->init();
            graph->nodes.push_back(node);

            link_t *link0 = new link_t;
            link0->a_id  = node->id;
            link0->b_id = geo[node->data->geometry];
            link0->a_slot = 0;
            link0->b_slot = 0;
            graph->links.push_back(link0);

            link_t *link1 = new link_t;
            link1->a_id  = node->id;
            link1->b_id = mat[node->data->material];
            link1->a_slot = 1;
            link1->b_slot = 0;
            graph->links.push_back(link1);

            y += interval;
        }
    }
}

void draw(graph_t *graph, const xtcore::Scene *scene, uintptr_t tex)
{
    ImVector<node_t*> *nodes = &(graph->nodes);
    ImVector<link_t*> *links = &(graph->links);

    graph->active_node = 0;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->ChannelsSplit(3);
    ImGui::BeginGroup();
    {

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding , ImVec2(1,1));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
        ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, (ImVec4)COL_BG);

        int flags = ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse|ImGuiWindowFlags_NoMove;
        ImGui::BeginChild("scrolling_region", ImVec2(0,0), true, flags);
        {
            ImVec2 offset = ImGui::GetCursorPos() - graph->scroll_position;

            { // Grid
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImVec2 wp = ImGui::GetCursorScreenPos();
                ImVec2 ws = ImGui::GetWindowSize();
                ImVec2 md = ImVec2(fmodf(offset.x, FLT_GRID_BLOCK), fmodf(offset.y, FLT_GRID_BLOCK));
                for (float x=md.x; x<ws.x; x+=FLT_GRID_BLOCK) draw_list->AddLine(ImVec2(x,0)+wp, ImVec2(x,ws.y)+wp, COL_GRID);
                for (float y=md.y; y<ws.y; y+=FLT_GRID_BLOCK) draw_list->AddLine(ImVec2(0,y)+wp, ImVec2(ws.x,y)+wp, COL_GRID);
                bool scene_is_scrolling = ImGui::IsMouseDragging(MOUSE_BUTTON_MIDDLE);
                if (scene_is_scrolling) graph->scroll_position = graph->scroll_position - ImGui::GetIO().MouseDelta;
	            ImGui::Image((void*)(uintptr_t)(tex), ImVec2(256, 256));
            }
            { // Nodes
                for (int i = 0; i < nodes->size(); ++i)
                {
                    ImVec2 circle_offset = ImVec2(NODE_SLOT_RADIUS / 2, 0);
                    node_t* node = nodes->Data[i];
                    bool hovered = node->draw(dl, offset, circle_offset);
                    if (hovered) graph->active_node = node->id;
                }
            }
            { // Links
                for (int i = 0; i < links->size(); ++i)
                {
                    link_t* link     = links->Data[i];
                    node_t* node_inp = nodes->Data[link->a_id];
                    node_t* node_out = nodes->Data[link->b_id];
                    ImVec2 p1 = offset + node_out->get_input_slot_position(link->b_slot);
                    ImVec2 p2 = offset + node_inp->get_output_slot_position(link->a_slot);

                    bool is_linked_item_hovered = (node_inp->id == graph->active_node)
                                                ||(node_out->id == graph->active_node);

                    ImColor col = is_linked_item_hovered ? COL_LINK_HOVERED : COL_LINK_NORMAL;

                    dl->AddBezierCurve(p1, p1+ImVec2(-50,0), p2+ImVec2(50,0), p2, col, 3.0f);
                }
            }
        }
        ImGui::EndChild();

        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
    }
    ImGui::EndGroup();

/*
    // Draw context menu
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8,8));
    if (ImGui::BeginPopup("context_menu"))
    {
        ImVec2 scene_pos = ImGui::GetMousePosOnOpeningCurrentPopup() - offset;
        if (ImGui::MenuItem("Add Camera")) {
            build(graph, scene);
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
*/

    // Scrolling
    dl->ChannelsMerge();
}

    } /* namespace graph */
} /* namespace gui */
