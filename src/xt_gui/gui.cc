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
#define MM_STR_PRESETS            "Presets"

typedef struct
{
    const size_t width;
    const size_t height;
    const char * description;
}resolution_t;

#define RES(w,h,s) { w, h, #w "x" #h " " s}

// https://en.wikipedia.org/wiki/Graphics_display_resolution
const resolution_t resolutions[] = {
      RES(  128,  768, "VStrip"   ) // Cubemaos
    , RES(  512, 3072, "VStrip"   )
    , RES( 1024, 6144, "VStrip"   )
    , RES( 1024, 1024, "Square"   ) // Square
    , RES( 2048, 2048, "Square"   )
    , RES( 4096, 4096, "Square"   )
    , RES(  640,  360, "nHD"      ) // High Definition
    , RES(  960,  540, "qHD"      )
    , RES( 1280,  720, "HD"       )
    , RES( 1600,  900, "HD+"      )
    , RES( 1920, 1080, "FHD"      )
    , RES( 2160, 1440, "FHD+"     )
    , RES( 2048, 1080, "DCI 2K"   )
    , RES( 2560, 1440, "QHD/WQHD" )
    , RES( 3200, 1800, "QHD+"     )
    , RES( 3440, 1440, "UWQHD"    )
    , RES( 3840, 1600, "UW4K"     )
    , RES( 3840, 2160, "4K UHD"   )
    , RES( 4096, 2160, "DCI 4K"   )
    , RES( 5120, 2160, "UW5K"     )
    , RES( 5120, 2880, "5K UHD+"  )
    , RES( 7680, 3200, "UW8K"     )
    , RES( 7680, 4320, "8K UHD"   )
    , RES(  160,  120, "QQVGA"    ) // Video Graphics Array
    , RES(  240,  160, "HQVGA"    )
    , RES(  320,  240, "QVGA"     )
    , RES(  400,  240, "WQVGA"    )
    , RES(  480,  320, "HVGA"     )
    , RES(  640,  480, "VGA/SD"   )
    , RES(  768,  480, "WVGA"     )
    , RES(  854,  480, "FWVGA"    )
    , RES(  800,  600, "SVGA"     )
    , RES(  960,  640, "DVGA"     )
    , RES( 1024,  576, "WSVGA 576")
    , RES( 1024,  600, "WSVGA 600")
    , RES( 1024,  768, "XGA"      ) // Extended Graphics Array
    , RES( 1366,  768, "WXGA"     )
    , RES( 1152,  864, "XGA+"     )
    , RES( 1440,  900, "WXGA+"    )
    , RES( 1280, 1024, "SXGA"     )
    , RES( 1400, 1050, "SXGA+"    )
    , RES( 1680, 1050, "WSXGA+"   )
    , RES( 1600, 1200, "UXGA"     )
    , RES( 1920, 1200, "WUXGA"    )
    , RES( 2048, 1152, "QWXGA"    ) // Quad Extended Graphics Array
    , RES( 2048, 1536, "QXGA"     )
    , RES( 2560, 1600, "WQXGA"    )
    , RES( 2560, 2048, "QSXGA"    )
    , RES( 3200, 2048, "WQSXGA"   )
    , RES( 3200, 2400, "QUXGA"    )
    , RES( 3840, 2400, "WQUXGA"   )
    , RES( 4096, 3072, "HXGA"     ) // Hyper Extended Graphics Array
    , RES( 5120, 3200, "WHXGA"    )
    , RES( 5120, 4096, "HSXGA"    )
    , RES( 6400, 4096, "WHSXGA"   )
    , RES( 6400, 4800, "HUXGA"    )
    , RES( 7680, 4800, "WHUXGA"   )
};




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

void mm_resolution (workspace_t *ws)
{
    size_t res_entries = sizeof(resolutions) / sizeof(resolution_t);
    int selected = -1;
    ImVec2 bd(100,0);

    ImGui::Text("Sensor");ImGui::Separator();
    ImGui::Columns(2, 0, false);
    ImGui::Text(MM_STR_PRESETS);
    ImGui::BeginChild("LST_RES", ImVec2(250, 100), true);
    for (size_t i = 0; i < res_entries; ++i) {
        const resolution_t *r = &resolutions[i];
        if (ImGui::Selectable(r->description, selected == i)) {
            ws->context.params.width  = r->width;
            ws->context.params.height = r->height;
            selected = i;
        }
    }
    ImGui::EndChild();
    ImGui::NextColumn();
    ImGui::NewLine();
    ImGui::Text("Resolution");
    textedit_int("Width"          , ws->context.params.width , 1, 1);
    textedit_int("Height"         , ws->context.params.height, 1, 1);
    ImGui::NewLine();
    ImGui::Checkbox("Clear Buffer", &(ws->clear_buffer));
    ImGui::Columns(1);
}

} /* namespace gui */
