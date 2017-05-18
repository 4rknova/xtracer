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

#define WORKSPACE_EXPLORER_WIDTH (320)


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

std::vector<workspace_t*> workspaces;

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

void render_main_menu(state_t *state)
{
	static bool _win_load = false;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open" , "CTRL+O")) { _win_load = true; }
            if (ImGui::MenuItem("Exit" , ""      )) { action::quit();   }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

	if (_win_load) { ImGui::OpenPopup("Load"); }

	if (ImGui::BeginPopupModal("Load"))
	{
		static char filepath[256];
		ImGui::InputText("Path", filepath, 256);

	    if (ImGui::Button("OK", ImVec2(120,0)))
		{
			workspace_t *ws = new workspace_t;
			ws->renderer = new Renderer();
			ws->source_file = filepath;
            ws->init();
			if (!action::load(ws)) workspaces.push_back(ws);
			ImGui::CloseCurrentPopup();
			_win_load = false;
		}
	    ImGui::SameLine();
	    if (ImGui::Button("Cancel", ImVec2(120,0)))
		{
			ImGui::CloseCurrentPopup();
			 _win_load = false;
		}
	    ImGui::EndPopup();
	}
}

void render_workspace_explorer(state_t *state)
{
    int flags = ImGuiWindowFlags_NoCollapse
			  | ImGuiWindowFlags_NoTitleBar
			  | ImGuiWindowFlags_NoResize
			  | ImGuiWindowFlags_NoMove;

	bool open;
	ImGui::SetNextWindowPos(ImVec2((float)1,(float)21), ImGuiSetCond_Appearing);
	ImGui::Begin("Workspace explorer", &(open), ImVec2(0,0), 0.5f, flags);
	ImGui::Image((void*)(uintptr_t)state->textures.logo, ImVec2(300,53));
	ImGui::Text("v%s", xtcore::get_version());
	ImGui::NewLine();
    ImVec2 res = ImVec2(WORKSPACE_EXPLORER_WIDTH,(float)state->window.height - 22);
    ImGui::SetWindowSize(res);

	ImGui::NewLine();
	ImGui::Text("Workspaces");
	ImGui::Separator();
	for (auto& i : workspaces) {
		std::string name = i->source_file.c_str();
    	if (ImGui::Button(name.c_str(), ImVec2(WORKSPACE_EXPLORER_WIDTH-20,20))) { i->is_visible = true; }
	}
	ImGui::End();
}

void render_workspaces(state_t *state)
{
	for (auto& i : workspaces) {
		workspace_t *ws = i;

		if (!ws || !(ws->is_visible)) continue;

        ws->update();

		int flags = ImGuiWindowFlags_NoCollapse;

		std::string name = i->source_file.c_str();
		ImGui::Begin(name.c_str(), &(ws->is_visible), ImVec2(0,0), 0.9f, flags);

	    if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("Zoom")) {
				if (ImGui::MenuItem(" 50%", NULL, false, true)) { ws->zoom_multiplier = 0.5f; }
				if (ImGui::MenuItem("100%", NULL, false, true)) { ws->zoom_multiplier = 1.0f; }
				if (ImGui::MenuItem("200%", NULL, false, true)) { ws->zoom_multiplier = 2.0f; }
		        ImGui::EndMenu();
			}
	    	ImGui::EndMenuBar();
		}

		ImGui::BeginChildFrame(0, ImVec2(200,250));
		ImGui::Text("Configuration");
		ImGui::Separator();
		ImGui::Text("Resolution    %lux%lu", ws->context.params.width, ws->context.params.height);
		ImGui::Text("Sypersampling %lu", ws->context.params.ssaa);
		ImGui::Text("Samples       %lu", ws->context.params.samples);
		ImGui::Text("Threads       %lu", ws->context.params.threads);
		ImGui::Text("R.Depth       %lu", ws->context.params.rdepth);
		ImGui::Text("Tile size     %lu", ws->context.params.tile_size);
		ImGui::NewLine();
	    if (ImGui::Button("Render", ImVec2(120,0))) { action::render(ws); }
		ImGui::EndChildFrame();
		ImGui::SameLine();
		ImGui::BeginChildFrame(1, ImVec2(200,250));
		ImGui::Text("Scene tree");
		ImGui::Separator();
		if (ImGui::TreeNode("root")) {
			if (ImGui::TreeNode("Cameras")) {
				CamCollection::iterator it = i->context.scene.m_cameras.begin();
				CamCollection::iterator et = i->context.scene.m_cameras.end();
				for (; it!=et; ++it) {
					if (ImGui::TreeNode((*it).first.c_str())) {
						ImGui::TreePop();
					}
				}
				ImGui::TreePop();
			}
			if (ImGui::TreeNode("Geometry")) {
				GeoCollection::iterator it = i->context.scene.m_geometry.begin();
				GeoCollection::iterator et = i->context.scene.m_geometry.end();
				for (; it!=et; ++it) {
					if (ImGui::TreeNode((*it).first.c_str())) {
						ImGui::TreePop();
					}
				}
				ImGui::TreePop();
			}
			if (ImGui::TreeNode("Materials")) {
				MatCollection::iterator it = i->context.scene.m_materials.begin();
				MatCollection::iterator et = i->context.scene.m_materials.end();
				for (; it!=et; ++it) {
					if (ImGui::TreeNode((*it).first.c_str())) {
						ImGui::TreePop();
					}
				}
				ImGui::TreePop();
			}
			if (ImGui::TreeNode("Objects")) {
				ObjCollection::iterator it = i->context.scene.m_objects.begin();
				ObjCollection::iterator et = i->context.scene.m_objects.end();
				for (; it!=et; ++it) {
					if (ImGui::TreeNode((*it).first.c_str())) {
						ImGui::Text("Geometry : %s", (*it).second->geometry.c_str());
						ImGui::Text("Material : %s", (*it).second->material.c_str());
						ImGui::TreePop();
					}
				}
				ImGui::TreePop();
			}
	        ImGui::TreePop();
	    }
		ImGui::EndChildFrame();

		ImGui::Image((void*)(uintptr_t)(ws->texture), ImVec2(ws->zoom_multiplier * ws->context.params.width
                                                                      , ws->zoom_multiplier * ws->context.params.height));
		ImGui::End();
	}
}


/*
void widget_main(state_t *state)
{
    int flags = ImGuiWindowFlags_NoTitleBar
              | ImGuiWindowFlags_NoSavedSettings
              | ImGuiWindowFlags_NoResize
              | ImGuiWindowFlags_NoMove;

	ImGui::Begin("Main", &(state->widgets.main.open), ImVec2(320,state->window.height), 0.5f, flags);

    ImVec2 res = ImVec2(320,(float)state->window.height);
    ImGui::SetWindowSize(res);

    ImGui::Image((void*)(uintptr_t)state->textures.logo, ImVec2(300,53));
    ImGui::NewLine();

	ImGui::Text("Scene: %s", state->context.scene.m_name.c_str());
    ImGui::Separator();
    ImGui::NewLine();

    ImGui::BeginGroup();
    {
    	if (ImGui::Button("Load", ImVec2(120,20))) {state->widgets.render.open = true; action_queue.push(ACTION_RENDER);}
        ImGui::SameLine();
    	if (ImGui::Button("Export", ImVec2(120,20))) {state->widgets.render.open = true; action_queue.push(ACTION_RENDER);}

    	if (ImGui::Button("Render", ImVec2(248,20))) {state->widgets.render.open = true; action_queue.push(ACTION_RENDER);}
	    if (ImGui::Button("Scene Explorer", ImVec2(248,20))) {state->widgets.scene_explorer.open  = true;                }
	    if (ImGui::Button("About", ImVec2(248,20))) {state->widgets.about.open  = true;                                  }
		ImGui::EndGroup();
	}
	ImGui::End();
}
void widget_explorer(state_t *state)
{
    if (!(state->widgets.scene_explorer.open)) return;

    int flags = ImGuiWindowFlags_NoCollapse;

	ImGui::Begin("Scene Explorer", &(state->widgets.scene_explorer.open), flags);
	ImGui::End();
}

void widget_render(state_t *state)
{
    static float multiplier = 1.f;

    int flags = ImGuiWindowFlags_AlwaysAutoResize
			  | ImGuiWindowFlags_MenuBar;

    if (!(state->widgets.render.open)) return;
	ImGui::Begin("Render", &(state->widgets.render.open), flags);

//    if (ImGui::BeginMenu("About")) { state->widgets.about.open = true; }
    if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("Zoom")) {
			if (ImGui::MenuItem(" 50%", NULL, false, true)) { multiplier = 0.5f; }
			if (ImGui::MenuItem("100%", NULL, false, true)) { multiplier = 1.0f; }
			if (ImGui::MenuItem("200%", NULL, false, true)) { multiplier = 2.0f; }
	        ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Process")) {
	        ImGui::EndMenu();
		}
	    ImGui::EndMenuBar();
	}


    ImGui::Text("Resolution    %lux%lu", state->context.params.width, state->context.params.height);
    ImGui::Text("Sypersampling %lu", state->context.params.ssaa);
    ImGui::Text("Samples       %lu", state->context.params.samples);
    ImGui::Text("Threads       %lu", state->context.params.threads);
    ImGui::Text("R.Depth       %lu", state->context.params.rdepth);
    ImGui::Text("Tile size     %lu", state->context.params.tile_size);
    ImGui::NewLine();
    ImGui::Image((void*)(uintptr_t)(state->textures.render), ImVec2(multiplier * state->context.params.width
                                                                  , multiplier * state->context.params.height));
    ImGui::End();
}

void widget_about(state_t *state)
{
    if (!state->widgets.about.open) return;

    int flags = ImGuiWindowFlags_AlwaysAutoResize
			  | ImGuiWindowFlags_NoCollapse;

	ImGui::Begin("About", &(state->widgets.about.open), flags);
    ImGui::End();
}
*/

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
    render_workspace_explorer(state);
    render_workspaces(state);

    ImGui::Render();
}

} /* namespace gui */
