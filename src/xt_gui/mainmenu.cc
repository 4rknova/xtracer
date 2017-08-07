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


#define LOG_HISTORY_SIZE (200)

struct resolution_t {
    const size_t  width;
    const size_t  height;
    const char   *description;
};

// https://en.wikipedia.org/wiki/Graphics_display_resolution
const resolution_t resolutions[] = {
// Cubemaps
      {  128,  768, "128x768"  }
    , {  512, 3072, "512x3072" }
    , { 1024, 6144, "1024x6144"}
// Square
    , { 1024, 1024, "1K"       }
    , { 2048, 2048, "2K"       }
    , { 4096, 4096, "4K"       }
// High Definition
    , {  640,  360, "nHD"      }
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

void wdg_log(bool &visible, state_t *state)
{
    ImGui::SetNextWindowPos(ImVec2(1.,21.), ImGuiSetCond_Appearing);
	if (ImGui::BeginPopupModal("Log", 0, WIN_FLAGS_SET_0)) {
        ImGui::SetWindowSize(ImVec2(state->window.width - 2., state->window.height - 23.));
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
//            if (ImGui::RadioButton((*it).first.c_str(), &e, ++cam_idx)) {
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


{
        size_t res_entries = sizeof(resolutions) / sizeof(resolution_t);
        ImVec2 bd(100,0);
        ImGui::Text("Sensor");ImGui::Separator();
        int selected = 0;

        ImGui::Columns(2, 0, false);
        ImGui::Text("Presets");
        ImGui::BeginChild("LST_RES", ImVec2(150, 100), true);
        for (size_t i = 0; i < res_entries; ++i) {
            const resolution_t *r = &resolutions[i];
            if (ImGui::Selectable(r->description, selected == i)) {
                p->width  = r->width;
                p->height = r->height;
                selected = i;
            }
        }
        ImGui::EndChild();
        ImGui::NextColumn();
        ImGui::NewLine();
        ImGui::Text("Resolution");
        textedit_int("Width"          , p->width , 1, 1);
        textedit_int("Height"         , p->height, 1, 1);
        ImGui::NewLine();
        ImGui::Checkbox("Clear Buffer", &(ws->clear_buffer));
        ImGui::Columns(1);
}
        ImGui::NewLine();
        ImGui::NewLine();
        ImGui::Text("Sampling");ImGui::Separator();
        textedit_int("Antialiasing"   , p->ssaa   , 1, 1);
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
        if (ImGui::Button("x0.2")) { ws->zoom_multiplier = 0.201f; } ImGui::SameLine();
        if (ImGui::Button("x0.5")) { ws->zoom_multiplier = 0.501f; } ImGui::SameLine();
        if (ImGui::Button("x1.0")) { ws->zoom_multiplier = 1.001f; } ImGui::SameLine();
        if (ImGui::Button("x2.0")) { ws->zoom_multiplier = 2.001f; } ImGui::SameLine();
        if (ImGui::Button("x4.0")) { ws->zoom_multiplier = 4.001f; }
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

            ImGui::Image((ImTextureID &)(i->texture), ImVec2(128, 128 * i->context.params.height / i->context.params.width));

            if ((state->workspace != i)
                && ImGui::MenuItem("Switch to this")) state->workspace = i;
            if (busy) {
                ImGui::Text("Rendering.. ");
                if (state->workspace->timer.get_time_in_sec() > 0.0) {
                    std::string timestr;
                    print_time_breakdown(timestr, state->workspace->timer.get_time_in_mlsec());
                    ImGui::Text("%s", timestr.c_str());
                }
                ImGui::ProgressBar(i->progress);
            }
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

#include <nimg/yuv4mpeg2.h>
void export_video(const char *filepath, workspace_t *ws)
{
    size_t w = ws->context.params.width;
    size_t h = ws->context.params.height;

    start_video(filepath, w, h, 25);

    nimg::Pixmap fb;
    xtcore::render::assemble(fb, ws->context);

    std::vector<float> rgb;
    for (size_t y = 0; y < h; ++y) {
        for (size_t x = 0; x < w; ++x) {
            nimg::ColorRGBAf pixel = fb.pixel(x,y);
            rgb.push_back(pixel.r());
            rgb.push_back(pixel.g());
            rgb.push_back(pixel.b());
        }
    }

    for (size_t i = 0; i < 60; ++i) {
        write_frame(filepath, w, h, &rgb[0]);
    }
}

void wdg_export(workspace_t *ws)
{
    if (ws->renderer) return;

    if (ImGui::BeginMenu("Export")) {
    	static char filepath[256];
	    ImGui::InputText("File", filepath, 256);

        if (strlen(filepath) > 0) {
            ImVec2 bd(52,0);
            if (ImGui::Button("HDR", bd)) action::export_hdr(filepath, ws); ImGui::SameLine();
            if (ImGui::Button("PNG", bd)) action::export_png(filepath, ws); ImGui::SameLine();
            if (ImGui::Button("TGA", bd)) action::export_tga(filepath, ws); ImGui::SameLine();
            if (ImGui::Button("BMP", bd)) action::export_bmp(filepath, ws);
            if (ImGui::Button("Y4M", bd)) export_video(filepath, ws);
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

        mm_create(state->workspace);

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

    wdg_log(_flag_popup_win_log, state);
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
