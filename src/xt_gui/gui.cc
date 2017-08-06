#include <xtcore/cam_perspective.h>
#include <xtcore/tile.h>
#include <imgui.h>
#include "imgui_extra.h"
#include "gui.h"

#define MM_STR_CREATE             "Create"
#define MM_STR_CAMERA             "Camera"
#define MM_STR_MATERIAL           "Material"
#define MM_STR_GEOMETRY           "Geometry"
#define MM_STR_ENTITY             "Entity"
#define MM_STR_CAM_PERSPECTIVE    "Perspective"
#define MM_STR_CAM_ODS            "ODS"
#define MM_STR_CAM_ERP            "ERP"
#define MM_STR_ADD                "add"
#define MM_STR_ORDER_RANDOM       "random"
#define MM_STR_ORDER_RAD_IN       "inwards"
#define MM_STR_ORDER_RAD_OUT      "outwards"
#define MM_STR_TILE_ORDER         "Tile order"

namespace gui {

void mm_create(workspace_t *ws)
{
    if (!ws) return;

    if (ImGui::BeginMenu(MM_STR_CREATE)) {
        // Creation is based on NCF node
        // Add button defines what is created
        not_yet_implemented();
        ImGui::EndMenu();
    }
}

void mm_tileorder(workspace_t *ws)
{
    int tile_order = (int)ws->tile_order;

    ImGui::Text(MM_STR_TILE_ORDER);
    ImGui::Separator();
    ImGui::Columns(3, 0, false);
    ImGui::RadioButton(MM_STR_ORDER_RANDOM , &tile_order, xtcore::render::TILE_ORDER_RANDOM);
    ImGui::NextColumn();
    ImGui::RadioButton(MM_STR_ORDER_RAD_IN , &tile_order, xtcore::render::TILE_ORDER_RADIAL_IN);
    ImGui::NextColumn();
    ImGui::RadioButton(MM_STR_ORDER_RAD_OUT, &tile_order, xtcore::render::TILE_ORDER_RADIAL_OUT);
    ImGui::Columns(1);
    ImGui::NewLine();

    ws->tile_order = (xtcore::render::TILE_ORDER)tile_order;
}

} /* namespace gui */
