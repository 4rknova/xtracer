#include <cstdio>
#include <string>
#include <queue>
#include <vector>
#include <xtcore/config.h>
#include <xtcore/log.h>
#include "ext/imgui.h"
#include "ext/stb_image.h"
#include "shader.h"
#include "state.h"
#include "action.h"
#include "logo.h"
#include "widgets.h"

#include <xtcore/plr_photonmapper/renderer.h>
#include <xtcore/plr_depth/depth.h>
#include <xtcore/plr_stencil/renderer.h>

#define GUI_SIDEPANEL_WIDTH        (325)
#define WORKSPACE_PROP_CONF_HEIGHT (300)
#define WORKSPACE_PROP_CONT_HEIGHT (100)

#define MIN(a,b) (a>b?b:a)
#define MAX(a,b) (a>b?a:b)


#define WIN_FLAGS_SET_0 ( ImGuiWindowFlags_NoCollapse \
                        | ImGuiWindowFlags_NoTitleBar \
                        | ImGuiWindowFlags_NoResize   \
                        | ImGuiWindowFlags_NoMove)

#define WIN_FLAGS_SET_1 WIN_FLAGS_SET_0 \
                        | ImGuiWindowFlags_HorizontalScrollbar


namespace gui {

static const char *postsdr_source_frag =
    "#version 130\n"
	"uniform sampler2D tex;\n"
	"uniform vec2 iWindowResolution;\n"
	"uniform vec2 iRenderResolution;\n"
	"uniform vec2 iTileSize;\n"
    "out vec4 Color;\n"
	"void main()\n"
	"{\n"
    "    vec2 uv = gl_FragCoord.xy / iWindowResolution.xy;\n"
    "float d = distance(vec2(iWindowResolution.x / 2., iWindowResolution.y / 1.2)\n"
	"                      , iWindowResolution - gl_FragCoord.xy) * (1./iWindowResolution.x-.03)*0.01;\n"
    "vec3 white = vec3(1);"
    "vec3 eightWhite = vec3(0.8, 0.8, 0.8);"
    "vec3 pink = vec3(0.11, 0.41, 0.91);"
    "vec3 blue = vec3(0.34, 0.36, 0.60);"
    "vec3 color = mix("
    "    mix(white, eightWhite, uv.x),"
    "    mix(pink, blue, uv.y),"
    "   	d"
    ");"
    "Color = vec4(.35*color, 1.);"

	"}\n";

size_t                    active_workspace = 0;
std::vector<workspace_t*> workspaces;

void slider_float(const char *name, float &val, float a, float b)
{
    float tmp = val;
    ImGui::SliderFloat(name, &tmp, a, b);
    val = tmp;
}

void slider_int(const char *name, size_t &val, size_t a, size_t b)
{
    int tmp = val;
    ImGui::SliderInt(name, &tmp, a, b);
    val = tmp;
}

void draw_group_logo(state_t *state)
{
    ImGui::BeginGroup();
    ImGui::Image((void*)(uintptr_t)state->textures.logo, ImVec2(300,53));
   	ImGui::Text("v%s", xtcore::get_version());
    ImGui::EndGroup();
}

void draw_group_render_controls(workspace_t *ws)
{
    static const ImVec2 btn_sz_zoom = ImVec2(50.f, 0.f);

    ImGui::BeginGroup();
    if (ImGui::Button("x0.5", btn_sz_zoom)) { ws->zoom_multiplier = 0.5f; } ImGui::SameLine();
    if (ImGui::Button("x1.0", btn_sz_zoom)) { ws->zoom_multiplier = 1.0f; } ImGui::SameLine();
    if (ImGui::Button("x2.0", btn_sz_zoom)) { ws->zoom_multiplier = 2.0f; }
    slider_float("Zoom"  , ws->zoom_multiplier , 0.5, 10.0);
    ImGui::EndGroup();
}

void draw_group_render_config(workspace_t *ws)
{
    ImGui::BeginGroup();
	ImGui::Text("Configuration");
	ImGui::Separator();
	ImGui::Text("Resolution    %lux%lu", ws->context.params.width, ws->context.params.height);
    slider_int("Supersampling"  , ws->context.params.ssaa     , 1, 10);
    slider_int("Recursion Depth", ws->context.params.rdepth   , 1, 10);
    slider_int("Tile Size"      , ws->context.params.tile_size, 1, MIN(ws->context.params.width, ws->context.params.height));
    slider_int("Samples"        , ws->context.params.samples  , 1, 32);
    slider_int("Threads"        , ws->context.params.threads  , 1, 32);
    ImGui::EndGroup();
}

void draw_render_scenegraph(workspace_t *ws)
{
    ImGui::BeginGroup();
    ImGui::Text("%s", ws->source_file.c_str());
	ImGui::Separator();
	if (ImGui::TreeNodeEx("root", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (ImGui::TreeNodeEx("Cameras", ImGuiTreeNodeFlags_DefaultOpen)) {
			CamCollection::iterator it = ws->context.scene.m_cameras.begin();
			CamCollection::iterator et = ws->context.scene.m_cameras.end();
			for (; it!=et; ++it) {
				if (ImGui::TreeNodeEx((*it).first.c_str())) {
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
		if (ImGui::TreeNodeEx("Geometry",ImGuiTreeNodeFlags_DefaultOpen)) {
			GeoCollection::iterator it = ws->context.scene.m_geometry.begin();
			GeoCollection::iterator et = ws->context.scene.m_geometry.end();
			for (; it!=et; ++it) {
				if (ImGui::TreeNodeEx((*it).first.c_str())) {
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
		if (ImGui::TreeNodeEx("Materials", ImGuiTreeNodeFlags_DefaultOpen)) {
			MatCollection::iterator it = ws->context.scene.m_materials.begin();
			MatCollection::iterator et = ws->context.scene.m_materials.end();
			for (; it!=et; ++it) {
				if (ImGui::TreeNodeEx((*it).first.c_str())) {
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
		if (ImGui::TreeNodeEx("Objects", ImGuiTreeNodeFlags_DefaultOpen)) {
			ObjCollection::iterator it = ws->context.scene.m_objects.begin();
			ObjCollection::iterator et = ws->context.scene.m_objects.end();
			for (; it!=et; ++it) {
				if (ImGui::TreeNodeEx((*it).first.c_str())) {
					ImGui::Text("Geometry : %s", (*it).second->geometry.c_str());
					ImGui::Text("Material : %s", (*it).second->material.c_str());
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
	    ImGui::TreePop();
	}
    ImGui::EndGroup();
}

void init(state_t *state)
{
	// Load logo
    int w, h, comp;
    unsigned char* image = stbi_load_from_memory(res_logo_png, res_logo_png_len, &w, &h, &comp, STBI_rgb_alpha);
    glGenTextures(1, &(state->textures.logo));
    glBindTexture(GL_TEXTURE_2D, state->textures.logo);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    stbi_image_free(image);
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

    postprg->set_uniform("iWindowResolution", NMath::Vector2f((float)state->window.width
                                                            , (float)state->window.height));

	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void render(workspace_t *ws, xtracer::render::IRenderer *r)
{
    delete ws->renderer;
    ws->renderer = r;
    action::render(ws);
}

void render_main_menu(state_t *state)
{
	static bool _flag_popup_win_load  = false;
	static bool _flag_popup_win_about = false;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open"  , "")) { _flag_popup_win_load  = true; }
            if (ImGui::MenuItem("About" , "")) { _flag_popup_win_about = true; }
            if (ImGui::MenuItem("Exit"  , "")) { action::quit();              }
            ImGui::EndMenu();
        }
        if (workspaces.size() > 0) {
            if  (ImGui::BeginMenu("Workspaces")) {
                for (auto& i : workspaces) {
            	    std::string name = i->source_file.c_str();
                   	if (ImGui::MenuItem(name.c_str())) state->workspace = i;
                }
                ImGui::EndMenu();
            }
            if  (ImGui::BeginMenu("Render")) {
                if (ImGui::MenuItem("Depth"         , "")) { render(state->workspace, new xtracer::renderer::depth::Renderer()); }
                if (ImGui::MenuItem("Stencil"       , "")) { render(state->workspace, new xtracer::renderer::stencil::Renderer()); }
                if (ImGui::MenuItem("Photon Mapper" , "")) { render(state->workspace, new Renderer()); }
                ImGui::EndMenu();
            }
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
                workspaces.push_back(ws);
                state->workspace = ws;
            }
			ImGui::CloseCurrentPopup();
			_flag_popup_win_load = false;
		}
	    ImGui::EndPopup();
	}
	if (ImGui::BeginPopupModal("About", 0, WIN_FLAGS_SET_0))
    {
        draw_group_logo(state);
        ImGui::NewLine();
	    if (ImGui::Button("OK", ImVec2(300,0))){
			ImGui::CloseCurrentPopup();
            _flag_popup_win_about = false;
        }
	    ImGui::EndPopup();
    }
}


void render_workspace(state_t *state)
{
        workspace_t *ws = state->workspace;

		if (!ws) return;

        ws->update();

		std::string name = ws->source_file.c_str();
	    ImGui::SetNextWindowPos(ImVec2((float)1,(float)21), ImGuiSetCond_Appearing);
        bool dummy;
		ImGui::Begin(name.c_str(), &(dummy), ImVec2(0,0), 0.3f, WIN_FLAGS_SET_0);

        ImVec2 res = ImVec2(GUI_SIDEPANEL_WIDTH,(float)state->window.height - 22);
        ImGui::SetWindowSize(res);


	    ImGui::BeginGroup();

        float sh = state->window.height - WORKSPACE_PROP_CONF_HEIGHT -  WORKSPACE_PROP_CONT_HEIGHT - 50 - 8;
        draw_render_scenegraph(ws);
        ImGui::EndGroup();
        ImGui::End();


	    ImGui::SetNextWindowPos(ImVec2((float)GUI_SIDEPANEL_WIDTH + 2,(float)21), ImGuiSetCond_Appearing);
		ImGui::Begin("Render Controls", &(dummy), ImVec2(0,0), 0.3f, WIN_FLAGS_SET_0);
        float rhs_w = state->window.width - GUI_SIDEPANEL_WIDTH - 4;
        ImGui::SetWindowSize(ImVec2(rhs_w, 250));
        draw_group_render_config(ws);
        ImGui::NewLine();
        draw_group_render_controls(ws);
		ImGui::End();

	    ImGui::SetNextWindowPos(ImVec2((float)GUI_SIDEPANEL_WIDTH + 2,(float)272), ImGuiSetCond_Appearing);
		ImGui::Begin("Render", &(dummy), ImVec2(0,0), 0.3f, WIN_FLAGS_SET_1);
        float ry = state->window.height - 273;
        float zx = ws->zoom_multiplier * ws->context.params.width;
        float zy = ws->zoom_multiplier * ws->context.params.height;
        ImGui::SetWindowSize(ImVec2(rhs_w, ry));
        ImGui::SetCursorPos(ImVec2(
              zx < rhs_w ? (rhs_w - zx) * 0.5f : 0.f
            , zy < ry    ? (ry    - zy) * 0.5f : 0.f ));
		ImGui::Image((void*)(uintptr_t)(ws->texture), ImVec2(ws->zoom_multiplier * ws->context.params.width,
		                                                     ws->zoom_multiplier * ws->context.params.height));
		ImGui::End();
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
        io.MouseWheel = wheel;
    }
}

void draw_widgets(state_t *state)
{
    render_background(state);
    render_main_menu(state);
    render_workspace(state);
    ImGui::Render();
}

} /* namespace gui */
