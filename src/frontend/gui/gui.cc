#include <stdlib.h>
#include <limits.h>
#include <xtcore/tile.h>
#include <xtcore/log.h>
#include <xtcore/timeutil.h>
#include <xtcore/integrator.h>
#include <xtcore/xtcore.h>
#include <xtcore/midi.h>
#include <nimg/yuv4mpeg2.h>
#include <stb/stb_image.h>
#include <imgui.h>

#include "shader.h"
#include "imgui_extra.h"
#include "config.h"
#include "action.h"
#include "util.h"
#include "fsutil.h"
#include "gui.h"
#include "logo.h"

#define STRINGIFY(A) #A
#define MIN(x,y) (x > y ? y : x)
#define MAX(x,y) (x > y ? x : y)

#define STRLEN_MAX (512)

#define STR_CREATE              "Create"
#define STR_CAMERA              "Camera"
#define STR_MATERIAL            "Material"
#define STR_GEOMETRY            "Geometry"
#define STR_ENTITY              "Entity"
#define STR_CAM_PERSPECTIVE     "Perspective"
#define STR_CAM_ODS             "ODS"
#define STR_CAM_ERP             "ERP"
#define STR_ADD                 "Add"
#define STR_ORDER_SCANLINE      "Scanline"
#define STR_ORDER_RANDOM        "Random"
#define STR_ORDER_RAD_IN        "Inwards"
#define STR_ORDER_RAD_OUT       "Outwards"
#define STR_TILE_ORDER          "Tile order"
#define STR_SAMPLE_DISTRIBUTION "Sample distribution"
#define STR_SAMPLE_DIST_GRID    "Grid Aligned"
#define STR_SAMPLE_DIST_RANDOM  "Monte Carlo"
#define STR_PRESETS             "Presets"
#define STR_SENSOR              "Sensor"
#define STR_WORKSPACES          "Workspaces"
#define STR_SCENE               "Scene"
#define STR_PREVIEW             "Preview"
#define STR_LOG                 "Log"
#define STR_FILTERS             "Filters"
#define STR_RMODE               "Rendering mode"
#define STR_RMODE_SINGLE        "Single frame"
#define STR_RMODE_CONTINUOUS    "Continuous"
#define STR_SHOW_TILE_UPDATES   "Show tile updates"
#define STR_SELECT              "Select"
#define STR_CLOSE               "Close"
#define STR_NETWORK             "Network"
#define STR_BROADCAST           "Broadcast"
#define STR_RECEIVE             "Receive"

enum ID {
      ID_PANEL_SIDE
    , ID_PANEL_MAIN
    , ID_PANEL_LOG
    , ID_PANEL_PREVIEW
};

#define SIZE_MAIN_MENU         20
#define SIZE_OFFSET            1

#define LOG_HISTORY_SIZE (10000)

#define WIN_FLAGS_SET_0 ( ImGuiWindowFlags_NoCollapse \
                        | ImGuiWindowFlags_NoTitleBar \
                        | ImGuiWindowFlags_NoResize   \
                        | ImGuiWindowFlags_NoMove)

#define WIN_FLAGS_SET_1 ( WIN_FLAGS_SET_0 \
                        | ImGuiWindowFlags_HorizontalScrollbar)

#define WIN_FLAGS_SET_2 ( ImGuiWindowFlags_NoCollapse \
                        | ImGuiWindowFlags_NoResize)

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
    , RES(  500,  500, "Square"   ) // Square
    , RES(  800,  800, "Square"   ) // Square
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

enum ACTION
{
      ACTION_NOP
    , ACTION_RENDER
    , ACTION_SCENE_SAVE
    , ACTION_SCENE_LOAD
    , ACTION_RENDER_SAVE
};

static const char *postsdr_source_frag =
#include "background.frag"

void apply_theme()
{
	ImGuiStyle& style = ImGui::GetStyle();
    style.WindowTitleAlign  = ImVec2(0.5, 0.5);

    style.Alpha             =  1.0f;
    style.FrameRounding     =  4.0f;
    style.IndentSpacing     = 12.0f;
	style.WindowRounding    =  5.3f;
	style.ScrollbarRounding =  2.0f;

	style.Colors[ImGuiCol_Text]                  = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
	style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.09f, 0.09f, 0.15f, 1.00f);
	style.Colors[ImGuiCol_ChildWindowBg]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.05f, 0.05f, 0.10f, 0.85f);
	style.Colors[ImGuiCol_Border]                = ImVec4(0.70f, 0.70f, 0.70f, 0.35f);
	style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.10f, 0.10f, 0.20f, 0.90f);
	style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.85f, 0.80f, 0.80f, 0.40f);
	style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.80f, 0.65f, 0.65f, 0.45f);
	style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.50f, 0.50f, 0.50f, 0.83f);
	style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.40f, 0.40f, 0.80f, 0.20f);
	style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.00f, 0.00f, 0.00f, 0.87f);
	style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.40f, 0.40f, 0.60f, 0.50f);
	style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.20f, 0.25f, 0.30f, 0.60f);
	style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.55f, 0.53f, 0.55f, 0.51f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.56f, 0.56f, 0.56f, 0.91f);
	style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.90f, 0.90f, 0.90f, 0.83f);
	style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.70f, 0.70f, 0.70f, 0.62f);
	style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.30f, 0.30f, 0.30f, 0.84f);
	style.Colors[ImGuiCol_Button]                = ImVec4(0.50f, 0.72f, 0.89f, 0.49f);
	style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.40f, 0.52f, 0.59f, 0.68f);
	style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.70f, 0.50f, 0.55f, 1.00f);
	style.Colors[ImGuiCol_Header]                = ImVec4(0.30f, 0.69f, 1.00f, 0.53f);
	style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.44f, 0.61f, 0.86f, 1.00f);
	style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.38f, 0.62f, 0.83f, 1.00f);
	style.Colors[ImGuiCol_Column]                = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	style.Colors[ImGuiCol_ColumnHovered]         = ImVec4(0.70f, 0.60f, 0.60f, 1.00f);
	style.Colors[ImGuiCol_ColumnActive]          = ImVec4(0.90f, 0.70f, 0.70f, 1.00f);
	style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(1.00f, 1.00f, 1.00f, 0.85f);
	style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
	style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
	style.Colors[ImGuiCol_PlotLines]             = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
	style.Colors[ImGuiCol_ModalWindowDarkening]  = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}

void init(state_t *state)
{
	// Load logo
    int w, h, comp;
    unsigned char* image = stbi_load_from_memory(__res_logo_png, __res_logo_png_len, &w, &h, &comp, STBI_rgb_alpha);
    glGenTextures(1, &(state->textures.logo));
    glBindTexture(GL_TEXTURE_2D, state->textures.logo);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    stbi_image_free(image);

    apply_theme();
}

void handle_io_kb(int key)
{
    ImGuiIO &io = ImGui::GetIO();
	printf("%i\n", key);
    io.AddInputCharacter(key);
}

void handle_io_ms(int x, int y, bool button_event, bool left, bool right, float wheel)
{
    ImGuiIO &io = ImGui::GetIO();
    io.MousePos = ImVec2((float)x, (float)y);
    if (button_event) {
        io.MouseDown[0] = left;
        io.MouseDown[1] = right;
        io.MouseWheel   = wheel;
    }
}

void render_background(state_t *state)
{
	static GLuint   vao     = 0;
	static GLuint   vbo     = 0;
	static Program *postprg = 0;

	if (!postprg) {
		Shader  *postsdr_frag;
        postsdr_frag = new Shader(GL_FRAGMENT_SHADER);
    	postprg = new Program;
    	postsdr_frag->set_source(postsdr_source_frag);
	    postprg->add_shader(postsdr_frag);
        postprg->link();
	}

	if (!vbo) {
		GLfloat verts[] = {
           -1,-1, 0,
            1,-1, 0,
            1, 1, 0,
           -1,-1, 0,
           -1, 1, 0,
            1, 1, 0,
		};

		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * sizeof(verts), &verts[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);
	}

    postprg->set_uniform("iWindowResolution"
        , NMath::Vector2f((float)state->window.width
                        , (float)state->window.height));

	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void draw_widgets(state_t *state)
{
    for (auto i : state->workspaces) i->update();
    render_background(state);
    render_main_menu(state);
    container(state);
    ImGui::Render();
}

void draw_edit_str(const char *label, std::string &value)
{
    static char _temp[STRLEN_MAX];
    memset(_temp, 0, STRLEN_MAX);
    strncpy(_temp, value.c_str(), STRLEN_MAX);
	ImGui::PushItemWidth(100);
    if (ImGui::InputText(label, _temp, STRLEN_MAX, ImGuiInputTextFlags_EnterReturnsTrue))
    {
        value = _temp;
    }
	ImGui::PopItemWidth();
}

void draw_edit_float(const char *label, float value)
{
    static float _temp = value;
	ImGui::PushItemWidth(100);
    if (ImGui::InputFloat(label, &_temp, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsDecimal)) {
		value = _temp;
	}
	ImGui::PopItemWidth();
}

void mm_resolution (workspace_t *ws)
{
    ImGui::Text("Resolution");
    textedit_int("Width" , ws->context.params.width , 1, 1);
    textedit_int("Height", ws->context.params.height, 1, 1);
    ImGui::NewLine();
    ImGui::Checkbox("Clear Buffer", &(ws->clear_buffer));
    ImGui::NewLine();
    size_t res_entries = sizeof(resolutions) / sizeof(resolution_t);
    ImVec2 bd(100,0);
    ImGui::Text(STR_PRESETS);
    ImGui::Columns(4, "LST_RES_COLUMNS", false);
    ImGui::Text("ID");     ImGui::NextColumn();
    ImGui::Text("Type");   ImGui::NextColumn();
    ImGui::Text("Width");  ImGui::NextColumn();
    ImGui::Text("Height"); ImGui::NextColumn();
    ImGui::Columns(1);
    ImGui::BeginChild("LST_RES", ImVec2(300, 500), true);
    {
        ImGui::Columns(4, "LST_RES_COLUMNS_INNER");
        for (size_t i = 0; i < res_entries; i++) {
            const resolution_t *r = &resolutions[i];
            char label[32];
            sprintf(label, "%02lu", i);
            bool selected = ((ws->context.params.width == r->width) && (ws->context.params.height == r->height));
            if (ImGui::Selectable(label, selected, ImGuiSelectableFlags_SpanAllColumns)) {
                ws->context.params.width  = r->width;
                ws->context.params.height = r->height;
            }
            ImGui::NextColumn();
            ImGui::Text("%s", r->description); ImGui::NextColumn();
            ImGui::Text("%lu", r->width);       ImGui::NextColumn();
            ImGui::Text("%lu", r->height);      ImGui::NextColumn();
        }
        ImGui::Columns(1);
    }
    ImGui::EndChild();
}

void mm_export(workspace_t *ws)
{
    if (ws->is_rendering()) return;

    if (ImGui::BeginMenu("Export")) {

    	static char filepath[512];
	    ImGui::InputText("File", filepath, 512);

        auto _button_lambda = [&, ws](const char *desc, action::IMG_FORMAT f,
            bool sameline) -> void {
            static const ImVec2 bd(52,0);
            if (ImGui::Button(desc, bd)) action::write(f, filepath, ws);
            if (sameline) { ImGui::SameLine();}
        };

        if (strlen(filepath) > 0) {
            _button_lambda("HDR", action::IMG_FORMAT_HDR, true);
            _button_lambda("PNG", action::IMG_FORMAT_PNG, true);
            _button_lambda("JPG", action::IMG_FORMAT_JPG, true);
            _button_lambda("BMP", action::IMG_FORMAT_BMP, true);
            _button_lambda("TGA", action::IMG_FORMAT_TGA, true);
/*
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
*/
        }
        ImGui::EndMenu();
    }
}

void mm_sampling(workspace_t *ws)
{
    xtcore::render::params_t *p = &(ws->context.params);
    textedit_int("Antialiasing"   , p->aa     , 1, 1);

    int sample_distribution = (int)(p->sample_distribution);
    ImGui::Text(STR_SAMPLE_DISTRIBUTION);
    ImGui::RadioButton(STR_SAMPLE_DIST_GRID  , &sample_distribution, xtcore::antialiasing::SAMPLE_DISTRIBUTION_GRID);
    ImGui::RadioButton(STR_SAMPLE_DIST_RANDOM, &sample_distribution, xtcore::antialiasing::SAMPLE_DISTRIBUTION_RANDOM);
    ImGui::NewLine();
    p->sample_distribution = (xtcore::antialiasing::SAMPLE_DISTRIBUTION)sample_distribution;

    textedit_int("Recursion Depth", p->rdepth , 1, 1);
    textedit_int("Samples"        , p->samples, 1, 1);
    ImGui::NewLine();

    int tile_order = (int)(p->tile_order);
    ImGui::Text(STR_TILE_ORDER);
    ImGui::RadioButton(STR_ORDER_SCANLINE, &tile_order, xtcore::render::TILE_ORDER_SCANLINE);
    ImGui::RadioButton(STR_ORDER_RANDOM  , &tile_order, xtcore::render::TILE_ORDER_RANDOM);
    ImGui::RadioButton(STR_ORDER_RAD_IN  , &tile_order, xtcore::render::TILE_ORDER_RADIAL_IN);
    ImGui::RadioButton(STR_ORDER_RAD_OUT , &tile_order, xtcore::render::TILE_ORDER_RADIAL_OUT);
    ImGui::NewLine();
    p->tile_order = (xtcore::render::TILE_ORDER)tile_order;
}

void mm_concurrency(workspace_t *ws)
{
    xtcore::render::params_t *p = &(ws->context.params);

    auto _button_lambda = [&, p](const char *desc, size_t sz, bool sameline = false) -> void {
        static const ImVec2 bd(30,0);
        if (ImGui::Button(desc, bd)) {
            p->tile_size = sz;
        }
        if (sameline) { ImGui::SameLine();}
    };

    _button_lambda(  "1",   1,  true);
    _button_lambda(  "2",   2,  true);
    _button_lambda(  "4",   4,  true);
    _button_lambda(  "8",   8,  true);
    _button_lambda( "16",  16,  true);
    _button_lambda( "32",  32,  true);
    _button_lambda( "64",  64,  true);
    _button_lambda("128", 128, false);

    textedit_int("Tile Size", p->tile_size, 1, 1, MIN(p->width, p->height));
    textedit_int("Threads", ws->context.params.threads, 1, 1);
}

void mm_camera(workspace_t *ws)
{
    auto it = ws->context.scene.m_cameras.begin();
    auto et = ws->context.scene.m_cameras.end();

    HASH_ID current_cam = ws->context.params.camera;
    int current_idx = 0;

    for (auto tt = ws->context.scene.m_cameras.begin(); (tt != et) && (*tt).first != current_cam; ++tt) { ++current_idx; }
    int selected_idx = current_idx;
    for (int idx = -1; it != et; ++it) {
        ImGui::RadioButton(xtcore::pool::str::get((*it).first), &selected_idx, ++idx);
    }

    if (current_idx != selected_idx) {
        auto tt = ws->context.scene.m_cameras.begin();
        for (int idx = 0; idx < selected_idx; ++idx) ++tt;
        ws->context.params.camera = (*tt).first;
    }
}


void wdg_conf(workspace_t *ws)
{
    if (ws->status == WS_STATUS_PROCESSING) return;
    if (ImGui::BeginMenu("Camera"     )) { mm_camera(ws);     ImGui::EndMenu(); }
    if (ImGui::BeginMenu("Sensor"     )) { mm_resolution(ws); ImGui::EndMenu(); }
}

void menu_workspaces(state_t *state)
{
    size_t idx = 0;

    for (auto& i : state->workspaces) {
	    std::string name = std::to_string(++idx) + ". " + i->source_file.c_str();

        if (ImGui::BeginMenu(name.c_str())) {
            bool busy = (i->is_rendering());
            if ((state->workspace != i) && ImGui::MenuItem("Switch to this")) state->workspace = i;
            if (busy) ImGui::Text("Rendering.. ");
            if (!busy && ImGui::MenuItem("Close")  ) action::close(state, i);
            ImGui::EndMenu();
        }
    }

    if (state->workspaces.size() > 0) {
        if (ImGui::MenuItem("Close all")) {
            for (auto& i : state->workspaces) {
                if (!(i->is_rendering())) action::close(state, i);
            }
        }
    }
}

void mm_network(state_t *state)
{
    if (ImGui::BeginMenu(STR_NETWORK)) {
        if (ImGui::MenuItem(STR_BROADCAST)) { action::broadcast(state); }
        if (ImGui::MenuItem(STR_RECEIVE  )) { action::listen   (state); }
        ImGui::EndMenu();
    }
}

void mm_integrator(workspace_t *ws)
{
    if (ImGui::BeginMenu("Render")) {
        int rmode = (int)ws->rmode;
        ImGui::RadioButton(STR_RMODE_SINGLE    , &rmode, WS_RMODE_SINGLE);
        ImGui::RadioButton(STR_RMODE_CONTINUOUS, &rmode, WS_RMODE_CONTINUOUS);
        ImGui::Checkbox(STR_SHOW_TILE_UPDATES, &(ws->show_tile_updates));
        ImGui::NewLine();
        ws->rmode = (WS_RMODE)rmode;

        if (ws->status != WS_STATUS_PROCESSING) {
            if (ImGui::BeginMenu("Sampling"   )) { mm_sampling(ws);    ImGui::EndMenu(); }
            if (ImGui::BeginMenu("Concurrency")) { mm_concurrency(ws); ImGui::EndMenu(); }
            if (ImGui::BeginMenu("Integrator")) {
                bool render = false;
                if (ImGui::MenuItem("Depth"     )) { render = true; ws->integrator = new xtcore::integrator::depth::Integrator();      }
                if (ImGui::MenuItem("Stencil"   )) { render = true; ws->integrator = new xtcore::integrator::stencil::Integrator();    }
                /*
                if (ImGui::MenuItem("Normal"    )) { render = true; ws->integrator = new xtcore::integrator::normal::Integrator();     }
                */
                if (ImGui::MenuItem("UV"        )) { render = true; ws->integrator = new xtcore::integrator::uv::Integrator();         }
                /*
                if (ImGui::MenuItem("Emission"  )) { render = true; ws->integrator = new xtcore::integrator::emission::Integrator();   }
                if (ImGui::MenuItem("Raytracer" )) { render = true; ws->integrator = new xtcore::integrator::raytracer::Integrator();  }
                */
                if (ImGui::MenuItem("Pathtracer")) { render = true; ws->integrator = new xtcore::integrator::pathtracer::Integrator(); }
                /*
                if (ImGui::MenuItem("Raymarcher")) { render = true; ws->integrator = new xtcore::integrator::raymarcher::Integrator(); }
                */
                if (render) action::render(ws);
                ImGui::EndMenu();
            }
        }
        ImGui::EndMenu();
    }
}

void mm_zoom(workspace_t *ws) {
    if (!ws) return;

    auto _button_lambda = [&, ws](const char *txt, float val, bool sameline = false) -> void {
        static const ImVec2 bd(50,0);
        if (ImGui::Button(txt, bd)) {
            ws->zoom_multiplier = val;
        }
        if (sameline) { ImGui::SameLine();}
    };

    if (ImGui::BeginMenu("View")) {
        _button_lambda("x0.1", 0.101f,  true);
        _button_lambda("x0.2", 0.201f,  true);
        _button_lambda("x0.4", 0.401f,  true);
        _button_lambda("x0.5", 0.501f,  true);
        _button_lambda("x1.0", 1.001f,  true);
        _button_lambda("x2.0", 2.001f, false);
        _button_lambda("x4.0", 4.001f,  true);
        _button_lambda("x5.0", 5.001f,  true);
        _button_lambda("x10.0",10.001f,false);

        textedit_float("Zoom", ws->zoom_multiplier, 0.1,0.1);
        ImGui::EndMenu();
    }
}

void mm_dialog_load(state_t *state, bool &is_active)
{
	static bool valid_file = false;
    static bool needs_refreshing = true;
    static util::filesystem::fsvec fsv;

    if (!is_active) return;

    ImGui::SetNextWindowPos(ImVec2(1.,21.), ImGuiSetCond_Appearing);
    ImGui::OpenPopup("Load");
	if (ImGui::BeginPopupModal("Load", 0, WIN_FLAGS_SET_0)) {

        const int maxlength = PATH_MAX;
		static char *filepath = 0;

        if (!filepath) {
            filepath = (char*)malloc(maxlength);
            memset(filepath, 0, maxlength);
        }

		if (filepath[0] == 0) {
            filepath[0] = '.';
            filepath[1] = 0;
        }


        if (needs_refreshing) {
            xtcore::Log::handle().post_debug("Fetching directory listing..");
            util::filesystem::ls(fsv, filepath);
            valid_file = util::filesystem::file_exists(filepath);
            needs_refreshing = false;
        }

	    if (ImGui::Button(valid_file ? "load" : "cancel", ImVec2(80,0))) {
			if (valid_file) {
				workspace_t *ws = new workspace_t;
	    		ws->source_file = filepath;
	   			action::load(ws);
    	        state->workspaces.push_back(ws);
        	    state->workspace = ws;
			}
			ImGui::CloseCurrentPopup();
			is_active = false;
		}
        ImGui::SameLine();
        ImGui::PushItemWidth(510);
		if (ImGui::InputText("##VAL", filepath, maxlength)) {
            needs_refreshing = true;
        }
        ImGui::PopItemWidth();

        ImGui::BeginChild("LST_FS", ImVec2(600, 250), true);
        for (auto f : fsv) {
            bool is_dir = (f.type == util::filesystem::FS_ENTRY_TYPE_DIRECTORY);
            if (is_dir) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5,0.5,0.5,1));

            bool is_parent     = (f.type == util::filesystem::FS_ENTRY_TYPE_SPECIAL_DIR_PARENT);
            bool is_directory  = (f.type == util::filesystem::FS_ENTRY_TYPE_DIRECTORY);
            bool is_scene_file = util::filesystem::has_extension(&f, "scn");
            if (!is_parent && !is_directory && !is_scene_file) continue;

            if (ImGui::Selectable(f.name.c_str(), false)) {
                realpath(f.path.c_str(), filepath);
                valid_file = !is_dir && !is_parent;
                needs_refreshing = true;
            }
            if (is_dir) ImGui::PopStyleColor();
        }
        ImGui::EndChild();

	    ImGui::EndPopup();
	}
}

void render_main_menu(state_t *state)
{
	static bool flag_dg_load = false;
	static bool flag_dg_info = false;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open")) { flag_dg_load = true; }
            if (state->workspaces.size() > 0) {
                if  (ImGui::BeginMenu("Workspaces")) {
                    menu_workspaces(state);
                    ImGui::EndMenu();
                }
            }
            xtcore::midi::devices_t devs;
            xtcore::midi::detect(&devs);
            if (devs.devices.size() > 0){
                if (ImGui::BeginMenu("Midi")) {
                    for(xtcore::midi::device_t &d : devs.devices) ImGui::MenuItem(d.name.c_str());
                    ImGui::EndMenu();
                }
            }
            if (ImGui::MenuItem("Exit" )) { action::quit();      }
            ImGui::EndMenu();
        }

        if (state->workspace && ImGui::BeginMenu("Workspace")) {
            gui::mm_zoom(state->workspace);
            wdg_conf(state->workspace);
            mm_integrator(state->workspace);
            mm_export(state->workspace);
            ImGui::EndMenu();
        }
        gui::mm_network(state);
        
        ImGui::SameLine(ImGui::GetWindowWidth()-60);
        if (ImGui::BeginMenu("About")) {
            ImGui::SameLine(ImGui::GetWindowWidth() - 515);
            ImGui::BeginGroup();
            ImGui::Image((void*)(uintptr_t)state->textures.logo, ImVec2(481,159));
            ImGui::SameLine(ImGui::GetWindowWidth() - 515);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 27.f);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 110.f);
            ImGui::Text("version %s", xtcore::get_version());
            ImGui::EndGroup();
            ImGui::Text("%s",xtcore::get_license());
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
    ImGui::PopStyleVar();

    mm_dialog_load(state, flag_dg_load);
}

void panel_log()
{
    static bool log_filter_dbg = false;
    static bool log_filter_msg = true;
    static bool log_filter_wrn = true;
    static bool log_filter_err = true;
    static bool log_autoscroll = true;

    ImGui::Columns(2, "ID_LOG", false);
    ImGui::SetColumnWidth(-1, 100);
    ImGui::BeginChild("LST_LOG_FILTERS", ImVec2(90, 125));
    ImGui::Text(STR_FILTERS);
    ImGui::Separator();
    ImGui::Checkbox("debug"   , &log_filter_dbg);
    ImGui::Checkbox("message" , &log_filter_msg);
    ImGui::Checkbox("warning" , &log_filter_wrn);
    ImGui::Checkbox("error"   , &log_filter_err);
    ImGui::EndChild();
    ImGui::NewLine();
    ImGui::Checkbox("autoscroll"   , &log_autoscroll);
    ImGui::NewLine();
    if (ImGui::Button("Clear", ImVec2(75,0))) { xtcore::Log::handle().clear(); }
    int line_count = 0;
    int sz = MIN(int(xtcore::Log::handle().get_size())-1, LOG_HISTORY_SIZE);
    ImGui::NextColumn();
    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    dl->AddRectFilled(p + ImVec2(1,1), p + ImGui::GetWindowSize(), ImColor(0.f,0.f,0.f,0.1f));
    ImGui::BeginChild(ID_PANEL_LOG, ImVec2(-1, ImGui::GetWindowHeight() - 10));
    ImGui::NewLine();
    for (int i = sz; i >= 0; --i) {
        xtcore::log_entry_t l = xtcore::Log::handle().get_entry(i);
        ImVec4 col;
        bool display = false;
        switch(l.type) {
            case xtcore::LOGENTRY_DEBUG   : col = ImVec4(0.3,0.8,1.0,1); display = log_filter_dbg; break;
            case xtcore::LOGENTRY_MESSAGE : col = ImVec4(1.0,1.0,1.0,1); display = log_filter_msg; break;
            case xtcore::LOGENTRY_WARNING : col = ImVec4(1.0,1.0,0.0,1); display = log_filter_wrn; break;
            case xtcore::LOGENTRY_ERROR   : col = ImVec4(1.0,0.0,0.0,1); display = log_filter_err; break;
        }
        if (display) {
            ++line_count;
            ImGui::Columns(2,"ID_LOG_CONTAINER");
            ImGui::SetColumnWidth(-1, 50);
            ImGui::TextColored(ImVec4(0.5,0.5,0.5,1), "%4i", line_count);
            ImGui::NextColumn();
            ImGui::TextColored(col, "%s", l.message.c_str());
            ImGui::Columns(1,"ID_LOG_CONTAINER");
        }
        if (log_autoscroll) ImGui::SetScrollHere();
    }
    ImGui::EndChild();
    ImGui::Columns(1, "ID_LOG");
}

bool panel_workspace(state_t *state, workspace_t *ws, int id, bool is_current = false)
{
    bool res = false;

    ImGui::BeginChildFrame(id, ImVec2(ImGui::GetWindowWidth() - 10, 135));

    float aspect  = ws->context.params.height / (float)ws->context.params.width;
    float thumb_w = (aspect < 1.f ? 128.f : (128.f / aspect));
    float thumb_h = (aspect > 1.f ? 128.f : (128.f * aspect));

    ImGui::Columns(3,"ID_WORKSPACE",false);
    ImGui::SetColumnWidth(-1,138);
    ImGui::SetCursorPos(ImVec2(3 + (128 - thumb_w) / 2, 3 + (128 - thumb_h) / 2));
    ImGui::Image((ImTextureID &)(ws->texture), ImVec2(thumb_w, thumb_h));
    ImGui::NextColumn();
    ImGui::SetColumnWidth(-1,75);
    ImVec2 button_sz(50,30);
    if (ImGui::Button(STR_SELECT, button_sz)) { res = true; }
    if (!(ws->is_rendering()) && ImGui::Button(STR_CLOSE , button_sz)) { action::close(state, ws); }
    ImGui::NextColumn();
    ImVec4 col(.8f,.8f,.8f,1.f);
    if (is_current) col = ImVec4(0.f,1.f,0.f,1.f);

    ImGui::TextColored(col, "%s", ws->source_file.c_str());
    ImGui::Text(ws->context.scene.m_name.c_str());
    ImGui::SameLine();
    ImGui::Text("v%s ", ws->context.scene.m_version.c_str());
    ImGui::Text(ws->context.scene.m_description.c_str());
    ImGui::Text("Resolution: %lux%lu", ws->context.params.width, ws->context.params.height);
    ImGui::Text("Active Cam: %s", ws->context.params.camera != HASH_ID_INVALID ? xtcore::pool::str::get(ws->context.params.camera) : "N/A");

    if (ws->timer.get_time_in_sec() > 0.0) {
        std::string timestr;
        print_time_breakdown(timestr, ws->timer.get_time_in_mlsec());
        ImGui::Text("%s", timestr.c_str());
    }
    if (ws->is_rendering()) {
        const ImVec2& size_arg = ImVec2(100.f,10.f);
        ImGui::ProgressBar(ws->progress, size_arg);
    }
    ImGui::Columns(1, "ID_WORKSPACE");
    ImGui::EndChildFrame();

    return res;
}

void panel_workspaces(state_t *state)
{
    static bool activity_filter_busy = true;
    static bool activity_filter_idle = true;

    ImGui::Columns(2, "LST_WORKSPACES", false);
    {
        ImGui::SetColumnWidth(-1,100);
        ImGui::BeginChild("LST_WSP_FILTERS", ImVec2(90, 125));
        ImGui::Text(STR_FILTERS);
        ImGui::Separator();
        ImGui::Checkbox("busy", &activity_filter_busy);
        ImGui::Checkbox("idle", &activity_filter_idle);
        ImGui::EndChild();
        ImGui::NextColumn();
        ImDrawList *dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        dl->AddRectFilled(p + ImVec2(1,1), p + ImGui::GetWindowSize(), ImColor(0.f,0.f,0.f,0.1f));
        int count = 0;
        for (auto& i : state->workspaces) {
            bool rendering = (i->is_rendering());
            if ( (!activity_filter_idle && !rendering)
              || (!activity_filter_busy &&  rendering)
            ) continue;

            ++count;

            bool is_current  = (state->workspace == i);
            bool set_current = panel_workspace(state, i, 100 + count, is_current);
            if (set_current) state->workspace = i;
        }
    }
    ImGui::Columns(1, "LST_WORKSPACES");
}

void panel_scene(workspace_t *ws)
{
    if (!ws) return;
    gui::graph::draw(&(ws->graph), &(ws->context.scene), ws->texture);
}

void panel_preview(workspace_t *ws)
{
    if (!ws) return;

    float zx = ws->zoom_multiplier * ws->context.params.width;
    float zy = ws->zoom_multiplier * ws->context.params.height;
    ImGui::BeginChild(ID_PANEL_PREVIEW, ImVec2(0,0), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    // Zooming
    ImGuiIO &io = ImGui::GetIO();
    if (io.KeyCtrl) {
        float z = ws->zoom_multiplier + io.MouseWheel * io.DeltaTime;
        ws->zoom_multiplier = z < 0.1 ? 0.1 : z;
    }
    // Middle mouse scroll
    if (ImGui::IsMouseDown(0)) {
        ImVec2 d = ImGui::GetIO().MouseDelta;
        ImGui::SetScrollX(ImGui::GetScrollX() - 10.f * ws->zoom_multiplier * io.DeltaTime * d.x);
        ImGui::SetScrollY(ImGui::GetScrollY() - 10.f * ws->zoom_multiplier * io.DeltaTime * d.y);
    }
    ImGui::SetCursorPos(ImVec2(MAX((ImGui::GetWindowWidth()  - zx) / 2.0f, 0.0f)
                             , MAX((ImGui::GetWindowHeight() - zy) / 2.0f, 0.0f)));
	ImGui::Image((void*)(uintptr_t)(ws->texture), ImVec2(zx, zy));
    ImGui::EndChild();
}

void container(state_t *state)
{
    enum TAB {
          TAB_LOG
        , TAB_WORKSPACES
        , TAB_SCENE
        , TAB_PREVIEW
    };

    static TAB current_tab = TAB_WORKSPACES;
    static bool dummy = true;

    workspace_t *ws = state->workspace;
    ImGui::SetNextWindowPos(ImVec2(SIZE_OFFSET, SIZE_MAIN_MENU + SIZE_OFFSET));
    ImGui::SetNextWindowSize(ImVec2(state->window.width - 2 * SIZE_OFFSET, state->window.height - SIZE_MAIN_MENU));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::Begin("Render", &(dummy), ImVec2(0,0), 0.1f, WIN_FLAGS_SET_0);
    ImGui::BeginGroup();
    dummy = (current_tab == TAB_WORKSPACES); if (      ImGui::GoxTab(STR_WORKSPACES, 75, &dummy)) current_tab = TAB_WORKSPACES;
    dummy = (current_tab == TAB_LOG);        if (      ImGui::GoxTab(STR_LOG       , 75, &dummy)) current_tab = TAB_LOG;
    dummy = (current_tab == TAB_SCENE);      if (ws && ImGui::GoxTab(STR_SCENE     , 75, &dummy)) current_tab = TAB_SCENE;
    dummy = (current_tab == TAB_PREVIEW);    if (ws && ImGui::GoxTab(STR_PREVIEW   , 75, &dummy)) current_tab = TAB_PREVIEW;
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginChild(ID_PANEL_MAIN);
    switch (current_tab) {
        case TAB_WORKSPACES : panel_workspaces(state); break;
        case TAB_LOG        : panel_log();             break;
        case TAB_SCENE      : panel_scene(ws);         break;
        case TAB_PREVIEW    : panel_preview(ws);       break;
    }
    ImGui::EndChild();
    ImGui::End();
    ImGui::PopStyleVar();
}

} /* namespace gui */
