#ifndef SINGULARITY_GRAPH_H_INCLUDED
#define SINGULARITY_GRAPH_H_INCLUDED

#include <vector>
#include <xtcore/strpool.h>

#include <xtcore/scene.h>
#include <xtcore/camera.h>
#include <xtcore/object.h>
#include <xtcore/geometry.h>
#include <xtcore/material.h>

#include "imgui_extra.h"

#define INVALID_ID          ( -1)
#define NODE_SLOT_RADIUS    (5.f)
#define NODE_WINDOW_PADDING ImVec2(10.f,10.f)

namespace gui {
    namespace graph {

struct node_t
{
    int         id;
    HASH_UINT64 name;
    ImVec2      position;
    ImVec2      size;
    size_t      inputs;
    size_t      outputs;

             node_t();
    virtual ~node_t();

    ImVec2 get_input_slot_position(int slot_no) const;
    ImVec2 get_output_slot_position(int slot_no) const;

    virtual void draw_properties() = 0;

    bool draw(ImDrawList *draw_list, ImVec2 offset, ImVec2 circle_offset, node_t *selected);
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

    void clear();

     graph_t();
    ~graph_t();
};

void build (graph_t *graph, const xtcore::Scene *scene);
void draw  (graph_t *graph, const xtcore::Scene *scene);

    } /* namespace graph */
} /* namespace gui */

#endif /* SINGULARITY_GRAPH_H_INCLUDED */
