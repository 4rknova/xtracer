#include <thread>
#include <string>
#include "opengl.h"
#include "ext/imgui.h"
#include <xtcore/config.h>
#include <xtcore/renderer.h>
#include <xtcore/renderer/stencil/renderer.h>
#include <xtcore/renderer/depth/renderer.h>
#include <xtcore/renderer/photon_mapper/renderer.h>
#include <xtcore/renderer.h>
#include <xtcore/log.h>
#include <xtcore/timeutil.h>
#include "imgui_extra.h"
#include "util.h"
#include "config.h"
#include "action.h"
#include "mainmenu.h"


#define LOG_HISTORY_SIZE (20)

namespace gui {

void wdg_log(bool &visible)
{
    ImGui::SetNextWindowPos(ImVec2(1.,21.), ImGuiSetCond_Appearing);
	if (ImGui::BeginPopupModal("Log", 0, WIN_FLAGS_SET_0)) {
        ImGui::SetWindowSize(ImVec2(600, 500));
    	if (ImGui::Button("close", ImVec2(100,0)))
    	{
    		ImGui::CloseCurrentPopup();
            visible = false;
    	}
        ImGui::SameLine();
        if (ImGui::Button("Clear", ImVec2(100,0))) { Log::handle().clear(); }
        for (int i = MIN(LOG_HISTORY_SIZE, Log::handle().get_size()-1); i >= 0; --i) {
            log_entry_t l = Log::handle().get_entry(i);
            ImVec4 col;
            switch(l.type) {
                case LOGENTRY_DEBUG   : col = ImVec4(0,0,1,1); break;
                case LOGENTRY_MESSAGE : col = ImVec4(1,1,1,1); break;
                case LOGENTRY_WARNING : col = ImVec4(1,1,0,1); break;
                case LOGENTRY_ERROR   : col = ImVec4(1,0,0,1); break;
            }
            ImGui::TextColored(col, "%s", l.message.c_str());
        }
    	ImGui::EndPopup();
    }
}


bool Picker(void* data, int n, const char** name)
{
	workspace_t *ws = (workspace_t*)data;
	auto it = ws->context.scene.m_cameras.begin();
	auto et = ws->context.scene.m_cameras.end();
	for (; it != et && n--!=0; ++it);
	*name = (*it).first.c_str();
  	return n < (int)(ws->context.scene.m_cameras.size());
}

void wdg_conf(workspace_t *ws)
{
    if (ws->renderer) return;

    if (ImGui::BeginMenu("Configuration")) {
        xtcore::render::params_t *p = &(ws->context.params);

        ImGui::Text("Camera");ImGui::Separator();
        static int selected = -1;
        if (ImGui::ListBox("Symbols", &selected, Picker, ws, (int)(ws->context.scene.m_cameras.size()))) {
            auto it = ws->context.scene.m_cameras.begin();
            auto et = ws->context.scene.m_cameras.end();
            int tmp = selected;
            for (; it != et && tmp-->0; ++it);
            ws->context.params.camera = (*it).first;
        }
        ImGui::NewLine();
        ImVec2 bd(100,0);
        ImGui::Text("Resolution");ImGui::Separator();
        if (ImGui::Button("320x240", bd)) { p->width =  320; p->height =  240; } ImGui::SameLine();
        if (ImGui::Button("640x480", bd)) { p->width =  640; p->height =  480; } ImGui::SameLine();
        if (ImGui::Button("800x600", bd)) { p->width =  800; p->height =  600; }
        if (ImGui::Button("1k"     , bd)) { p->width = 1024; p->height = 1024; } ImGui::SameLine();
        if (ImGui::Button("2k"     , bd)) { p->width = 2048; p->height = 2048; } ImGui::SameLine();
        if (ImGui::Button("4k"     , bd)) { p->width = 4096; p->height = 4096; }
        if (ImGui::Button("720p"   , bd)) { p->width = 1280; p->height =  720; } ImGui::SameLine();
        if (ImGui::Button("1080p"  , bd)) { p->width = 1920; p->height = 1080; } ImGui::SameLine();
        if (ImGui::Button("1440p"  , bd)) { p->width = 2560; p->height = 1440; }
        textedit_int("Width"          , p->width , 1, 1);
        textedit_int("Height"         , p->height, 1, 1);
        ImGui::NewLine();
        ImGui::Text("Sampling");ImGui::Separator();
        textedit_int("Antialiasing"   , p->ssaa   , 1, 1);
        textedit_int("Recursion Depth", p->rdepth , 1, 1);
        textedit_int("Samples"        , p->samples, 1, 1);
        ImGui::NewLine();
        ImGui::Text("Concurrency");ImGui::Separator();
        textedit_int("Tile Size"      , p->tile_size, 1, 1, MIN(p->width, p->height));
        textedit_int("Threads"        , ws->context.params.threads, 1, 1);
        ImGui::EndMenu();
    }
}

void wdg_renderer(workspace_t *ws)
{
    if (ws->renderer) return;

    if (ImGui::BeginMenu("Render")) {
        bool render = false;
        if (ImGui::MenuItem("Depth"        )) { render = true; ws->renderer = new xtcore::renderer::depth::Renderer(); }
        if (ImGui::MenuItem("Stencil"      )) { render = true; ws->renderer = new xtcore::renderer::stencil::Renderer(); }
        if (ImGui::MenuItem("Photon Mapper")) { render = true; ws->renderer = new Renderer(); }
        if (render) action::render(ws);
        ImGui::EndMenu();
    }
}

void wdg_zoom(workspace_t *ws)
{
    if (!ws) return;

    if (ImGui::BeginMenu("View")) {
        ImGui::Text("Zoom");
        ImGui::Separator();
        if (ImGui::Button("x0.2")) { ws->zoom_multiplier = 0.2f; } ImGui::SameLine();
        if (ImGui::Button("x0.5")) { ws->zoom_multiplier = 0.5f; } ImGui::SameLine();
        if (ImGui::Button("x1.0")) { ws->zoom_multiplier = 1.0f; } ImGui::SameLine();
        if (ImGui::Button("x2.0")) { ws->zoom_multiplier = 2.0f; } ImGui::SameLine();
        if (ImGui::Button("x4.0")) { ws->zoom_multiplier = 4.0f; }
        textedit_float("Zoom", ws->zoom_multiplier, 0.1,0.1);
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

            if (ImGui::MenuItem("Switch to this")) state->workspace = i;
            if (busy) {
                ImGui::ProgressBar(i->progress);
            }
            if (!(i->renderer) && ImGui::MenuItem("Close")  ) action::close(state, i);
            ImGui::EndMenu();
        }
    }
}

void wdg_export(workspace_t *ws)
{
    if (ws->renderer) return;

    if (ImGui::BeginMenu("Export")) {
        if (ws->time > 0.0) {
            std::string timestr;
            print_time_breakdown(timestr, ws->time);
            ImGui::Text("Render time: %s", timestr.c_str());
            ImGui::NewLine();
        }

    	static char filepath[256];
	    ImGui::InputText("File", filepath, 256);

        if (strlen(filepath) > 0) {
            ImVec2 bd(52,0);
            if (ImGui::Button("HDR", bd)) action::export_hdr(filepath, ws); ImGui::SameLine();
            if (ImGui::Button("PNG", bd)) action::export_png(filepath, ws); ImGui::SameLine();
            if (ImGui::Button("TGA", bd)) action::export_tga(filepath, ws); ImGui::SameLine();
            if (ImGui::Button("BMP", bd)) action::export_bmp(filepath, ws);
        }
        ImGui::EndMenu();
    }
}

void render_main_menu(state_t *state)
{
	static bool _flag_popup_win_load  = false;
	static bool _flag_popup_win_about = false;
	static bool _flag_popup_win_log   = false;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open")) { _flag_popup_win_load  = true; }
            if (state->workspaces.size() > 0) {
                if  (ImGui::BeginMenu("Workspaces")) {
                    menu_workspaces(state);
                    ImGui::EndMenu();
                }
            }
            if (ImGui::MenuItem("Exit"  )) { action::quit();               }
            ImGui::EndMenu();
        }
        if (state->workspace && ImGui::BeginMenu("Workspace")) {
            wdg_zoom(state->workspace);
            wdg_conf(state->workspace);
            wdg_renderer(state->workspace);
            wdg_export(state->workspace);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window")) {
            if (ImGui::MenuItem("Log"  )) { _flag_popup_win_log   = true; }
            if (ImGui::MenuItem("About")) { _flag_popup_win_about = true; }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // Popups
	if (_flag_popup_win_load ) { ImGui::OpenPopup("Load");  }
	if (_flag_popup_win_about) { ImGui::OpenPopup("About"); }
    if (_flag_popup_win_log  ) { ImGui::OpenPopup("Log");   }

    wdg_log(_flag_popup_win_log);
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
        ImGui::Text("%s",xtcore::get_license());
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
