#ifndef XT_GRAPH_H_INCLUDED
#define XT_GRAPH_H_INCLUDED

#include <vector>
#include <xtcore/strpool.h>

#include <xtcore/scene.h>
#include <xtcore/camera.h>
#include <xtcore/object.h>
#include <xtcore/math/surface.h>
#include <xtcore/material.h>
#include <xtcore/sampler_col.h>
#include "imgui_extra.h"

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

    bool draw(ImDrawList *dl, ImVec2 offset, ImVec2 circle_offset);
    void init();
};

struct node_cam_t : public node_t
{
    xtcore::asset::ICamera *data;
    virtual void draw_properties();
};

struct node_obj_t : public node_t
{
    xtcore::asset::Object *data;
    virtual void draw_properties();
};

struct node_mat_t : public node_t
{
    xtcore::asset::IMaterial *data;
    virtual void draw_properties();
};

struct node_geo_t : public node_t
{
    xtcore::asset::ISurface *data;
    virtual void draw_properties();
};

struct node_col_t : public node_t
{
    xtcore::sampler::SolidColor *data;
    virtual void draw_properties();
};

struct node_numf_t : public node_t
{
    float *data;
    virtual void draw_properties();
};

struct link_t
{
    int a_id;
    int b_id;
    int a_slot;
    int b_slot;

    link_t();
    link_t(int input_idx, int input_slot, int output_idx, int output_slot);
};

struct graph_t {
    ImVector<node_t*> nodes;
    ImVector<link_t*> links;

    ImVec2 scroll_position;
    int active_node;

    void clear();

     graph_t();
    ~graph_t();
};

void build (graph_t *graph, const xtcore::Scene *scene);
void draw  (graph_t *graph, const xtcore::Scene *scene);

    } /* namespace graph */
} /* namespace gui */

#endif /* XT_GRAPH_H_INCLUDED */
