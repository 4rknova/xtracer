#include <xtcore/cam_perspective.h>
#include "ext/imgui.h"
#include "imgui_extra.h"
#include "mm_create.h"

#define MM_STR_CREATE           "Create"
#define MM_STR_CAMERA           "Camera"
#define MM_STR_MATERIAL         "Material"
#define MM_STR_GEOMETRY         "Geometry"
#define MM_STR_ENTITY           "Entity"
#define MM_STR_CAM_PERSPECTIVE  "Perspective"
#define MM_STR_CAM_ODS          "ODS"
#define MM_STR_CAM_ERP          "ERP"
#define MM_STR_ADD              "add"

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

} /* namespace gui */
