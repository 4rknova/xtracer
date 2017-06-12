#include <thread>
#include "opengl.h"
#include "ext/imgui.h"
#include <xtcore/config.h>
#include <xtcore/renderer.h>
#include <xtcore/renderer/stencil/renderer.h>
#include <xtcore/renderer/depth/renderer.h>
#include <xtcore/renderer/photon_mapper/renderer.h>
#include <xtcore/renderer.h>
#include "imgui_extra.h"
#include "util.h"
#include "config.h"
#include "action.h"
#include "mainmenu.h"

namespace gui {

void render(workspace_t *ws, xtcore::render::IRenderer *r)
{
    ws->renderer = r;
    action::render(ws);
}

void wdg_config(workspace_t *ws)
{
    textedit_int("Width"          , ws->context.params.width , 1);
    textedit_int("Height"         , ws->context.params.height, 1);
    ImGui::NewLine();
    textedit_int("Tile Size"      , ws->context.params.tile_size, 1
        , MIN(ws->context.params.width, ws->context.params.height));
    textedit_int("Supersampling"  , ws->context.params.ssaa   , 1);
    textedit_int("Recursion Depth", ws->context.params.rdepth , 1);
    textedit_int("Samples"        , ws->context.params.samples, 1);
    textedit_int("Threads"        , ws->context.params.threads, 1);
}

void render_main_menu(state_t *state)
{
	static bool _flag_popup_win_load  = false;
	static bool _flag_popup_win_about = false;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open"  , "")) { _flag_popup_win_load  = true; }
            if (ImGui::MenuItem("Exit"  , "")) { action::quit();               }
            ImGui::EndMenu();
        }
        if (state->workspaces.size() > 0) {
            if  (ImGui::BeginMenu("Workspaces")) {
                for (auto& i : state->workspaces) {
            	    std::string name = i->source_file.c_str();
                   	if (ImGui::MenuItem(name.c_str())) state->workspace = i;
                }
                ImGui::EndMenu();
            }
        }
        if (state->workspace) {
            if (ImGui::BeginMenu("Render")) {
                if (state->workspace->renderer) {
                    ImGui::Text("Rendering..");
                    ImGui::ProgressBar(state->workspace->progress);
                } else {
                    if (ImGui::BeginMenu("Configuration")) {
                        wdg_config(state->workspace);
                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu("Renderer")) {
                        if (ImGui::MenuItem("Depth"         , "")) { render(state->workspace, new xtcore::renderer::depth::Renderer()); }
                        if (ImGui::MenuItem("Stencil"       , "")) { render(state->workspace, new xtcore::renderer::stencil::Renderer()); }
                        if (ImGui::MenuItem("Photon Mapper" , "")) { render(state->workspace, new Renderer()); }
                        ImGui::EndMenu();
                    }
                }
                ImGui::EndMenu();
            }
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About" , "")) { _flag_popup_win_about = true; }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

	if (_flag_popup_win_load ) { ImGui::OpenPopup("Load");  }
	if (_flag_popup_win_about) { ImGui::OpenPopup("About"); }

    ImGui::SetNextWindowPos(ImVec2(1.,21.), ImGuiSetCond_Appearing);
	if (ImGui::BeginPopupModal("Load", 0, WIN_FLAGS_SET_0))
	{
		static char filepath[256];
		ImGui::InputText("File", filepath, 256);

        size_t path_given = (strlen(filepath) > 0);
        const char *text_button = (path_given > 0 ? "OK" : "Cancel");

	    if (ImGui::Button(text_button, ImVec2(300,0)))
		{
            if (path_given) {
    			workspace_t *ws = new workspace_t;
		    	ws->source_file = filepath;
                ws->init();
    			action::load(ws);
                state->workspaces.push_back(ws);
                state->workspace = ws;
            }
			ImGui::CloseCurrentPopup();
			_flag_popup_win_load = false;
		}
	    ImGui::EndPopup();
	}
	if (ImGui::BeginPopupModal("About", 0, WIN_FLAGS_SET_2))
    {
        ImGui::SameLine(ImGui::GetWindowWidth() - 425);
        ImGui::BeginGroup();
        ImGui::NewLine();
        ImGui::Image((void*)(uintptr_t)state->textures.logo, ImVec2(300,53));
        ImGui::Text("v%s", xtcore::get_version());
        ImGui::NewLine();
        ImGui::EndGroup();
        ImGui::NewLine();
        ImGui::Text(xtcore::get_license());
        ImGui::NewLine();
        ImGui::NewLine();
        ImGui::SameLine(ImGui::GetWindowWidth() - 100);
	    if (ImGui::Button("OK", ImVec2(75,0))) {
			ImGui::CloseCurrentPopup();
            _flag_popup_win_about = false;
        }
        ImGui::NewLine();
	    ImGui::EndPopup();
    }
}

} /* namespace gui */
