#include <cstdio>
#include <string>
#include <queue>
#include <vector>
#include <imgui.h>
#include <stb_image.h>

#include "gui.h"
#include "config.h"
#include "shader.h"
#include "state.h"
#include "action.h"
#include "logo.h"
#include "widgets.h"
#include "theme.h"
#include "mainmenu.h"
#include "imgui_extra.h"

namespace gui {

static const char *postsdr_source_frag =
    "#version 130\n"
	"uniform vec2 iWindowResolution;"
	"uniform vec2 iRenderResolution;"
    "out vec4 Color;"
	"void main() {"
    "vec2 uv = gl_FragCoord.xy / iWindowResolution.xy;"
    "float f = distance(vec2(iWindowResolution.x / 2., iWindowResolution.y / 3.)"
	"                      , iWindowResolution - gl_FragCoord.xy) * (1./iWindowResolution.x-.02)*0.01;"
    "vec3 a = vec3(1)"
    "   , b = vec3(0.8, 0.8, 0.8)"
    "   , c = vec3(0.11, 0.41, 0.91)"
    "   , d = vec3(0.34, 0.36, 0.90);"
    "vec3 color = 0.35 * mix(mix(a,b,uv.y),mix(c,d,uv.x),f);"
    "Color = vec4(color, 1);"
	"}";


void draw_group_render_controls(workspace_t *ws)
{
    static const ImVec2 btn_sz_zoom = ImVec2(50.f, 0.f);

    ImGui::BeginGroup();
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

    apply_theme();
}
/*
void render_workspace(state_t *state)
{

        workspace_t *ws = state->workspace;

		if (!ws) return;

        for (auto i : state->workspaces) i->update();

		std::string name = ws->source_file.c_str();
	    ImGui::SetNextWindowPos(ImVec2((float)1,(float)21), ImGuiSetCond_Appearing);
        bool dummy;
		ImGui::Begin(name.c_str(), &(dummy), ImVec2(0,0), 0.3f, WIN_FLAGS_SET_0);

        ImVec2 res = ImVec2(GUI_SIDEPANEL_WIDTH,(float)state->window.height - 22);
        ImGui::SetWindowSize(res);


	    ImGui::BeginGroup();

        float sh = state->window.height - WORKSPACE_PROP_CONF_HEIGHT -  WORKSPACE_PROP_CONT_HEIGHT - 50 - 8;

        ImGui::EndGroup();
        ImGui::End();

        float rhs_w = state->window.width - GUI_SIDEPANEL_WIDTH - 4;
	    ImGui::SetNextWindowPos(ImVec2((float)GUI_SIDEPANEL_WIDTH + 2,22), ImGuiSetCond_Appearing);
		ImGui::Begin("Render", &(dummy), ImVec2(0,0), 0.3f, WIN_FLAGS_SET_1);
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

    //render_workspace(state);
    ImGui::Render();
}

} /* namespace gui */
