#include <xtcore/cam_perspective.h>
#include <xtcore/config.h>
#include <xtcore/tile.h>
#include <xtcore/log.h>
#include <xtcore/timeutil.h>
#include <xtcore/renderer.h>
#include <xtcore/renderer/stencil/renderer.h>
#include <xtcore/renderer/depth/renderer.h>
#include <xtcore/renderer/photon_mapper/renderer.h>
#include <nimg/yuv4mpeg2.h>
#include <imgui.h>

#include "imgui_extra.h"
#include "config.h"
#include "action.h"
#include "util.h"
#include "fsutil.h"
#include "gui.h"

#define MM_STR_CREATE             "Create"
#define MM_STR_CAMERA             "Camera"
#define MM_STR_MATERIAL           "Material"
#define MM_STR_GEOMETRY           "Geometry"
#define MM_STR_ENTITY             "Entity"
#define MM_STR_CAM_PERSPECTIVE    "Perspective"
#define MM_STR_CAM_ODS            "ODS"
#define MM_STR_CAM_ERP            "ERP"
#define MM_STR_ADD                "Add"
#define MM_STR_ORDER_RANDOM       "Random"
#define MM_STR_ORDER_RAD_IN       "Inwards"
#define MM_STR_ORDER_RAD_OUT      "Outwards"
#define MM_STR_TILE_ORDER         "Tile order"
#define MM_STR_PRESETS            "Presets"
#define MM_STR_SENSOR             "Sensor"

#define LOG_HISTORY_SIZE (200)

typedef struct
{
    const size_t width;
    const size_t height;
    const char * description;
} resolution_t;

#define RES(w,h,s) { w, h, s }

// https://en.wikipedia.org/wiki/Graphics_display_resolution
const resolution_t resolutions[] = {
      RES(  128,  768, "VStrip"   ) // Cubemaps
    , RES(  512, 3072, "VStrip"   )
    , RES( 1024, 6144, "VStrip"   )
    , RES(  768,  128, "HStrip"   )
    , RES( 3072,  512, "HStrip"   )
    , RES( 6144, 1024, "HStrip"   )
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

    ImGui::Text(MM_STR_SENSOR);ImGui::Separator();
    ImGui::Text(MM_STR_PRESETS);
    ImGui::BeginChild("LST_RES", ImVec2(300, 100), true);
    for (size_t i = 0; i < res_entries; ++i) {
        const resolution_t *r = &resolutions[i];
        ImGui::Columns(4, "ID_RESOLUTIONS", false);
        if (ImGui::Selectable("apply", selected == i)) {
            ws->context.params.width  = r->width;
            ws->context.params.height = r->height;
            selected = i;
        }
        ImGui::NextColumn();
        ImGui::Text("%i", r->width);
        ImGui::NextColumn();
        ImGui::Text("%i", r->height);
        ImGui::NextColumn();
        ImGui::Columns(1, "ID_RESOLUTIONS");
    }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginGroup();
        ImGui::Text("Resolution");
        textedit_int("Width"          , ws->context.params.width , 1, 1);
        textedit_int("Height"         , ws->context.params.height, 1, 1);
        ImGui::NewLine();
        ImGui::Checkbox("Clear Buffer", &(ws->clear_buffer));
    ImGui::EndGroup();
}

void mm_export(workspace_t *ws)
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
            if (ImGui::Button("Y4M", bd)) {
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
        }
        ImGui::EndMenu();
    }
}

void mm_renderer(workspace_t *ws)
{
    if (ws->renderer) return;

    if (ImGui::BeginMenu("Render")) {
        bool render = false;
        if (ImGui::MenuItem("Depth"    )) { render = true; ws->renderer = new xtcore::renderer::depth::Renderer(); }
        if (ImGui::MenuItem("Stencil"  )) { render = true; ws->renderer = new xtcore::renderer::stencil::Renderer(); }
        if (ImGui::MenuItem("Raytracer")) { render = true; ws->renderer = new Renderer(); }
        if (render) action::render(ws);
        ImGui::EndMenu();
    }
}

void mm_zoom(workspace_t *ws) {
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

void mm_workspaces(state_t *state)
{
    static bool activity_filter_running = true;
    static bool activity_filter_idle    = true;
    ImGui::Checkbox("running", &activity_filter_running); ImGui::SameLine();
    ImGui::Checkbox("idle"   , &activity_filter_idle);
    ImGui::NewLine();

    int count = 0;
    for (auto& i : state->workspaces) {
        bool rendering = (i->renderer != 0);
        if (
            (!activity_filter_idle    && !rendering)
         || (!activity_filter_running &&  rendering)
        ) continue;

        ++count;
        ImGui::BeginChildFrame(120 + count, ImVec2(300,135));

        float aspect  = i->context.params.height / (float)i->context.params.width;
        float thumb_w = (aspect < 1.f ? 128.f : (128.f / aspect));
        float thumb_h = (aspect > 1.f ? 128.f : (128.f * aspect));

        ImGui::Columns(2,"ID_WORKSPACE",false);
            ImGui::SetColumnWidth(-1,138);
            ImGui::Image((ImTextureID &)(i->texture), ImVec2(thumb_w, thumb_h));
        ImGui::NextColumn();
            ImGui::Text("%s", i->source_file.c_str());
            ImGui::Separator();

            if ((i->progress > 0.f) && (i->progress < 1.f)) ImGui::ProgressBar(i->progress);
            else if (state->workspace->timer.get_time_in_sec() > 0.0) {
                std::string timestr;
                print_time_breakdown(timestr, i->timer.get_time_in_mlsec());
                ImGui::Text("%s", timestr.c_str());
            }

            if (state->workspace != i && ImGui::Button("Select")) state->workspace = i;
        ImGui::Columns(1);
        ImGui::EndChildFrame();
    }
}

void mm_dialog_load(state_t *state, bool &is_active)
{
    if (!is_active) return;

    ImGui::SetNextWindowPos(ImVec2(1.,21.), ImGuiSetCond_Appearing);
    ImGui::OpenPopup("Load");
	if (ImGui::BeginPopupModal("Load", 0, WIN_FLAGS_SET_0)) {

        const int maxlength = 4096;
		static char filepath[maxlength];

		if (filepath[0] == 0) filepath[0] = '.';

		util::filesystem::fsvec fsv;
        util::filesystem::ls(fsv, filepath);

		bool valid_file = true;

	    if (ImGui::Button(valid_file ? "load" : "cancel", ImVec2(80,0))) {
			if (valid_file) {
				workspace_t *ws = new workspace_t;
	    		ws->source_file = filepath;
            	ws->init();
	   			action::load(ws);
    	        state->workspaces.push_back(ws);
        	    state->workspace = ws;
			}
			ImGui::CloseCurrentPopup();
			is_active = false;
		}

        ImGui::SameLine();
		ImGui::InputText("File", filepath, maxlength);

        ImGui::BeginChild("LST_FS", ImVec2(450, 250), true);
        for (auto f : fsv) {
            bool is_dir = (f.type == util::filesystem::FS_ENTRY_TYPE_DIRECTORY);
            if (is_dir) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5,0.5,0.5,1));
            if (ImGui::Selectable(f.name.c_str(), false)) {
                strncpy(filepath, f.path.c_str(), f.path.size());
                if (!(is_dir)) valid_file = true;
            }
            if (is_dir) ImGui::PopStyleColor();
        }
        ImGui::EndChild();

	    ImGui::EndPopup();
	}
}

void mm_dialog_info(state_t *state, bool &is_active)
{
    if (!is_active) return;

    ImGui::OpenPopup("About");
	if (ImGui::BeginPopupModal("About", 0, WIN_FLAGS_SET_2)) {
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
            is_active = false;
        }
        ImGui::NewLine();
	    ImGui::EndPopup();
    }
}

void mm_dialog_log(state_t *state, bool &is_active)
{
    static bool log_filter_dbg = true;
    static bool log_filter_msg = true;
    static bool log_filter_wrn = true;
    static bool log_filter_err = true;

    if (!is_active) return;

    ImGui::SetNextWindowPos(ImVec2(1.,21.), ImGuiSetCond_Appearing);

    ImGui::OpenPopup("Log");
	if (ImGui::BeginPopupModal("Log", 0, WIN_FLAGS_SET_0)) {
        ImGui::SetWindowSize(ImVec2(state->window.width - 2., state->window.height - 23.));

        if (ImGui::Button("close", ImVec2(100,0))) {
            is_active = false;
    		ImGui::CloseCurrentPopup();
    	}
        ImGui::SameLine();
        if (ImGui::Button("Clear", ImVec2(100,0))) { Log::handle().clear(); }
        ImGui::SameLine();
        ImGui::Checkbox("debug"   , &log_filter_dbg); ImGui::SameLine();
        ImGui::Checkbox("message" , &log_filter_msg); ImGui::SameLine();
        ImGui::Checkbox("warning" , &log_filter_wrn); ImGui::SameLine();
        ImGui::Checkbox("error"   , &log_filter_err);

        ImGui::NewLine();
        int line_count = 0;
        int sz = MIN(Log::handle().get_size() - 1, LOG_HISTORY_SIZE);
        for (int i = sz; i >= 0; --i) {
            log_entry_t l = Log::handle().get_entry(i);
            ImVec4 col;
            bool display = false;
            switch(l.type) {
                case LOGENTRY_DEBUG   : col = ImVec4(0,0,1,1); display = log_filter_dbg; break;
                case LOGENTRY_MESSAGE : col = ImVec4(1,1,1,1); display = log_filter_msg; break;
                case LOGENTRY_WARNING : col = ImVec4(1,1,0,1); display = log_filter_wrn; break;
                case LOGENTRY_ERROR   : col = ImVec4(1,0,0,1); display = log_filter_err; break;
            }
            if (display) {
                ++line_count;
                ImGui::Columns(2,0);
                ImGui::SetColumnWidth(-1, 30);
                ImGui::TextColored(ImVec4(0.5,0.5,0.5,1), "%i", line_count);
                ImGui::NextColumn();
                ImGui::TextColored(col, "%s", l.message.c_str());
                ImGui::Columns(1);
            }
        }
    	ImGui::EndPopup();
    }
}

} /* namespace gui */
