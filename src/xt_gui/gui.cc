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

#define RES(w,h,s) { w, h, #w " x " #h s}

// https://en.wikipedia.org/wiki/Graphics_display_resolution
const resolution_t resolutions[] = {
    // Cubemaps
      {  128,  768, " 128 x  768, VStrip" }
    , {  512, 3072, " 512 x 3072, VStrip" }
    , { 1024, 6144, "1024 x 6144, VStrip" }
    // Square
    , { 1024, 1024, "1024 x 1024, Square" }
    , { 2048, 2048, "2048 x 2048, Square" }
    , { 4096, 4096, "4096 x 4096, Square" }
    // High Definition
    , {  640,  360, " 640  360 nHD"      }
    , {  960,  540, "qHD"      }
    , { 1280,  720, "HD"       }
    , { 1600,  900, "HD+"      }
    , { 1920, 1080, "FHD"      }
    , { 2160, 1440, "FHD+"     }
    , { 2048, 1080, "DCI 2K"   }
    , { 2560, 1440, "QHD/WQHD" }
    , { 3200, 1800, "QHD+"     }
    , { 3440, 1440, "UWQHD"    }
    , { 3840, 1600, "UW4K"     }
    , { 3840, 2160, "4K UHD"   }
    , { 4096, 2160, "DCI 4K"   }
    , { 5120, 2160, "UW5K"     }
    , { 5120, 2880, "5K UHD+"  }
    , { 7680, 3200, "UW8K"     }
    , { 7680, 4320, "8K UHD"   }
    // Video Graphics Array
    , {  160,  120, "QQVGA"    }
    , {  240,  160, "HQVGA"    }
    , {  320,  240, "QVGA"     }
    , {  400,  240, "WQVGA"    }
    , {  480,  320, "HVGA"     }
    , {  640,  480, "VGA/SD"   }
    , {  768,  480, "WVGA"     }
    , {  854,  480, "FWVGA"    }
    , {  800,  600, "SVGA"     }
    , {  960,  640, "DVGA"     }
    , { 1024,  576, "WSVGA 576"}
    , { 1024,  600, "WSVGA 600"}
    // Extended Graphics Array
    , { 1024,  768, "XGA"      }
    , { 1366,  768, "WXGA"     }
    , { 1152,  864, "XGA+"     }
    , { 1440,  900, "WXGA+"    }
    , { 1280, 1024, "SXGA"     }
    , { 1400, 1050, "SXGA+"    }
    , { 1680, 1050, "WSXGA+"   }
    , { 1600, 1200, "UXGA"     }
    , { 1920, 1200, "WUXGA"    }
    // Quad Extended Graphics Array
    , { 2048, 1152, "QWXGA"    }
    , { 2048, 1536, "QXGA"     }
    , { 2560, 1600, "WQXGA"    }
    , { 2560, 2048, "QSXGA"    }
    , { 3200, 2048, "WQSXGA"   }
    , { 3200, 2400, "QUXGA"    }
    , { 3840, 2400, "WQUXGA"   }
    // Hyper Extended Graphics Array
    , { 4096, 3072, "HXGA"     }
    , { 5120, 3200, "WHXGA"    }
    , { 5120, 4096, "HSXGA"    }
    , { 6400, 4096, "WHSXGA"   }
    , { 6400, 4800, "HUXGA"    }
    , { 7680, 4800, "WHUXGA"   }
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
    ImGui::BeginChild("LST_RES", ImVec2(150, 100), true);
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
