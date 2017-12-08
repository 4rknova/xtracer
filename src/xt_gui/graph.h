#ifndef SINGULARITY_GRAPH_H_INCLUDED
#define SINGULARITY_GRAPH_H_INCLUDED

#include <imgui.h>

#define INVALID_ID (-1)

struct node_t
{
    int     ID;
    char    Name[32];
    ImVec2  Pos, Size;
    float   Value;
    ImVec4  Color;
    int     InputsCount, OutputsCount;

    node_t(int id, const char* name, const ImVec2& pos, float value, const ImVec4& color, int inputs_count, int outputs_count)
    {
        ID = id;
        strncpy(Name, name, 31);
        Name[31] = 0;
        Pos = pos;
        Value = value;
        Color = color;
        InputsCount = inputs_count;
        OutputsCount = outputs_count;
    }

    ImVec2 GetInputSlotPos(int slot_no) const
    {
        return ImVec2(Pos.x, Pos.y + Size.y * ((float)slot_no+1) / ((float)InputsCount+1));
    }

    ImVec2 GetOutputSlotPos(int slot_no) const
    {
        return ImVec2(Pos.x + Size.x, Pos.y + Size.y * ((float)slot_no+1) / ((float)OutputsCount+1));
    }
};

struct link_t
{
    int InputIdx, InputSlot, OutputIdx, OutputSlot;

    link_t(int input_idx, int input_slot, int output_idx, int output_slot)
    {
        InputIdx   = input_idx;
        InputSlot  = input_slot;
        OutputIdx  = output_idx;
        OutputSlot = output_slot;
    }

    link_t()
    {
        reset();
    }

    void reset()
    {
        InputIdx   = INVALID_ID;
        InputSlot  = INVALID_ID;
        OutputIdx  = INVALID_ID;
        OutputSlot = INVALID_ID;
    }
};

struct graph_t {
    ImVector<node_t> nodes;
    ImVector<link_t> links;

    int active_node;

    graph_t()
        : active_node(INVALID_ID)
    {}
};

void draw_nlist(bool* opened, graph_t *graph);
void draw_graph(bool* opened, graph_t *graph, link_t *link);

#endif /* SINGULARITY_GRAPH_H_INCLUDED */
