#include <cstdio>
#include <queue>
#include <xtcore/config.h>
#include "ext/imgui.h"
#include "ext/imgui_impl_glut.h"
#include "widgets.h"

namespace gui {

std::queue<ACTION> action_queue;

ACTION get_action()
{
	if (action_queue.size()) {
		ACTION a = action_queue.front();
		action_queue.pop();
		return a;
	} else return ACTION_NOP;
}

void init(state_t *state)
{
    ImGui_ImplGLUT_Init();
    state->widgets.main.open   = true;
    state->widgets.render.open = true;
}

void widget_main(state_t *state)
{
    int flags = ImGuiWindowFlags_NoTitleBar
              | ImGuiWindowFlags_NoSavedSettings
              | ImGuiWindowFlags_NoResize
              | ImGuiWindowFlags_NoMove;

	ImGui::Begin("Main", &(state->widgets.main.open), ImVec2(320,state->window.height), 0.5f, flags);

    ImGui::Image((void*)(uintptr_t)state->textures.logo, ImVec2(300,53));
    ImGui::NewLine();

	ImGui::Text("Scene: %s", state->ctx.scene->m_name.c_str());
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
	if (ImGui::TreeNode("root")) {
		if (ImGui::TreeNode("Cameras")) {
			CamCollection::iterator it = state->ctx.scene->m_cameras.begin();
			CamCollection::iterator et = state->ctx.scene->m_cameras.end();
			for (; it!=et; ++it) {
				if (ImGui::TreeNode((*it).first.c_str())) {
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Geometry")) {
			GeoCollection::iterator it = state->ctx.scene->m_geometry.begin();
			GeoCollection::iterator et = state->ctx.scene->m_geometry.end();
			for (; it!=et; ++it) {
				if (ImGui::TreeNode((*it).first.c_str())) {
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Materials")) {
			MatCollection::iterator it = state->ctx.scene->m_materials.begin();
			MatCollection::iterator et = state->ctx.scene->m_materials.end();
			for (; it!=et; ++it) {
				if (ImGui::TreeNode((*it).first.c_str())) {
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Objects")) {
			ObjCollection::iterator it = state->ctx.scene->m_objects.begin();
			ObjCollection::iterator et = state->ctx.scene->m_objects.end();
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
			if (ImGui::MenuItem("Render", NULL, false, true)) { action_queue.push(ACTION_RENDER); }
	        ImGui::EndMenu();
		}
	    ImGui::EndMenuBar();
	}


    ImGui::Text("Resolution    %lux%lu", state->ctx.params.width, state->ctx.params.height);
    ImGui::Text("Sypersampling %lu", state->ctx.params.ssaa);
    ImGui::Text("Samples       %lu", state->ctx.params.samples);
    ImGui::Text("Threads       %lu", state->ctx.params.threads);
    ImGui::Text("R.Depth       %lu", state->ctx.params.rdepth);
    ImGui::Text("Tile size     %lu", state->ctx.params.tile_size);
    ImGui::NewLine();
    ImGui::Image((void*)(uintptr_t)(state->textures.render), ImVec2(multiplier * state->ctx.params.width
                                                                  , multiplier * state->ctx.params.height));
    ImGui::End();
}

void widget_about(state_t *state)
{
    if (!state->widgets.about.open) return;

    int flags = ImGuiWindowFlags_AlwaysAutoResize
			  | ImGuiWindowFlags_NoCollapse;

	ImGui::Begin("About", &(state->widgets.about.open), flags);
    ImGui::Text("xtracer v%s", xtcore::get_version());
    ImGui::Separator();
    ImGui::Text("By Nikos Papadopoulos.");
    ImGui::Text("xtracer is licensed under the BSD 3-clause License, see LICENSE for more information.");
    ImGui::End();
}

void handle_io_kb(unsigned char key)
{
    ImGuiIO &io = ImGui::GetIO();
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
	ImGui_ImplGLUT_NewFrame(state->window.width
                          , state->window.height);

#ifdef DEBUG
    ImGui::ShowMetricsWindow();
#endif

    widget_main(state);
    widget_render(state);
    widget_explorer(state);
    widget_about(state);

    ImGui::Render();
}

} /* namespace gui */
