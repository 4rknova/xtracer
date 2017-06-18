#include <cstdio>
#include <string>
#include <queue>
#include <vector>
#include "ext/imgui.h"
#include "ext/stb_image.h"
#include "config.h"
#include "shader.h"
#include "state.h"
#include "action.h"
#include "logo.h"
#include "widgets.h"
#include "theme.h"
#include "mainmenu.h"
#include "imgui_extra.h"

#define GUI_SIDEPANEL_WIDTH        (325)
#define WORKSPACE_PROP_CONF_HEIGHT (300)
#define WORKSPACE_PROP_CONT_HEIGHT (100)

#define STRLEN_MAX (512)

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

void draw_render_scenegraph(state_t *state)
{
    workspace_t *ws = state->workspace;

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
					for (size_t s = 0; s < (*it).second->get_scalar_count(); ++s) {
						std::string _temp;
						float val = (*it).second->get_scalar_by_index(s,&_temp);
						draw_edit_float(_temp.c_str(), val);
					}
					for (size_t s = 0; s < (*it).second->get_sampler_count(); ++s) {
						std::string _temp;
						(*it).second->get_sampler_by_index(s,&_temp);
						if (ImGui::TreeNodeEx(_temp.c_str())) {
							ImGui::TreePop();
						}
					}
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
                    draw_edit_str("Geometry", (*it).second->geometry);
                    draw_edit_str("Material", (*it).second->material);
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

    apply_theme();
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
        draw_render_scenegraph(state);
        ImGui::EndGroup();
        ImGui::End();

        float rhs_w = state->window.width - GUI_SIDEPANEL_WIDTH - 4;
	    ImGui::SetNextWindowPos(ImVec2((float)GUI_SIDEPANEL_WIDTH + 2,22), ImGuiSetCond_Appearing);
		ImGui::Begin("Render", &(dummy), ImVec2(0,0), 0.3f, WIN_FLAGS_SET_1);
        float ry = state->window.height - 23;
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
