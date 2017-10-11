#include <thread>
#include <string>
#include "opengl.h"
#include <xtcore/config.h>
#include <xtcore/renderer.h>
#include <xtcore/renderer/stencil/renderer.h>
#include <xtcore/renderer/depth/renderer.h>
#include <xtcore/renderer/photon_mapper/renderer.h>
#include <xtcore/renderer.h>
#include <xtcore/log.h>
#include <xtcore/timeutil.h>
#include <imgui.h>
#include "imgui_extra.h"

// Main menu modules
#include "gui.h"
#include "util.h"
#include "config.h"
#include "action.h"
#include "mainmenu.h"

namespace gui {

void wdg_conf(workspace_t *ws)
{
    if (ws->renderer) return;

    if (ImGui::BeginMenu("Configuration")) {
        xtcore::render::params_t *p = &(ws->context.params);

        ImGui::Text("Camera");ImGui::Separator();

        static int index = 0, selected = -1;
        auto it = ws->context.scene.m_cameras.begin();
        auto et = ws->context.scene.m_cameras.end();

{
        ImGui::Columns(2, 0, false);
        int i=0,selected = 0;
        ImGui::BeginChild("LST_CAM", ImVec2(150, 100), true);
        for (int cam_idx = 0; it != et; ++it) {
            ++i;
            if (ImGui::Selectable((*it).first.c_str(), selected == i)) {
                ws->context.params.camera = (*it).first;
                i = selected;
            }
        }
        ImGui::EndChild();
        ImGui::NextColumn();
        ImGui::Text("Current");
        ImGui::Text(ws->context.params.camera.c_str());
        ImGui::Columns(1);
        ImGui::NewLine();
}


        mm_resolution(ws);

        ImGui::NewLine();
        ImGui::Text("Sampling");ImGui::Separator();
        textedit_int("Antialiasing"   , p->aa     , 1, 1);
        textedit_int("Recursion Depth", p->rdepth , 1, 1);
        textedit_int("Samples"        , p->samples, 1, 1);
        ImGui::NewLine();
        mm_tileorder(ws);
        ImGui::Text("Concurrency");ImGui::Separator();
        ImVec2 bd(30,0);
        if (ImGui::Button("8"  , bd)) { p->tile_size =   8; } ImGui::SameLine();
        if (ImGui::Button("16" , bd)) { p->tile_size =  16; } ImGui::SameLine();
        if (ImGui::Button("32" , bd)) { p->tile_size =  32; } ImGui::SameLine();
        if (ImGui::Button("64" , bd)) { p->tile_size =  64; } ImGui::SameLine();
        if (ImGui::Button("128", bd)) { p->tile_size = 128; }
        textedit_int("Tile Size"      , p->tile_size, 1, 1, MIN(p->width, p->height));
        textedit_int("Threads"        , ws->context.params.threads, 1, 1);
        ImGui::NewLine();
        ImGui::EndMenu();
    }
}

void menu_workspaces(state_t *state)
{
    size_t idx = 0;

    for (auto& i : state->workspaces) {
	    std::string name = std::to_string(++idx) + ". " + i->source_file.c_str();

        if (ImGui::BeginMenu(name.c_str())) {
            bool busy = (i->renderer);
            if ((state->workspace != i) && ImGui::MenuItem("Switch to this")) state->workspace = i;
            if (busy) ImGui::Text("Rendering.. ");
            if (!(i->renderer) && ImGui::MenuItem("Close")  ) action::close(state, i);
            ImGui::EndMenu();
        }
    }

    if (state->workspaces.size() > 0) {
        if (ImGui::MenuItem("Close all")) {
            for (auto& i : state->workspaces) {
                if (!(i->renderer)) action::close(state, i);
            }
        }
    }
}

void render_main_menu(state_t *state)
{
	static bool flag_dg_load = false;
	static bool flag_dg_info = false;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open")) { flag_dg_load = true; }
            if (state->workspaces.size() > 0) {
                if  (ImGui::BeginMenu("Workspaces")) {
                    menu_workspaces(state);
                    ImGui::EndMenu();
                }
            }
            if (ImGui::MenuItem("About")) { flag_dg_info = true; }
            if (ImGui::MenuItem("Exit" )) { action::quit();              }
            ImGui::EndMenu();
        }

        if (state->workspace && ImGui::BeginMenu("Workspace")) {
            gui::mm_zoom(state->workspace);
            wdg_conf(state->workspace);
            gui::mm_renderer(state->workspace);
            gui::mm_export(state->workspace);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    gui::mm_dialog_load(state, flag_dg_load);
	gui::mm_dialog_info(state, flag_dg_info);
}

} /* namespace gui */
