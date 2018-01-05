#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>
#include <xtcore/xtcore.h>
#include "config.h"
#include "state.h"
#include "widgets.h"
#include "profiler.h"

gui::state_t state;

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
}

int main(int argc, char** argv)
{
    std::string title;
    title  = argv[0];
    title += " v";
    title += xtcore::get_version();

    // Setup window
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(WINDOW_DEFAULT_WIDTH
                                        , WINDOW_DEFAULT_HEIGHT
                                        , title.c_str()
                                        , NULL, NULL);
    glfwMakeContextCurrent(window);

    /* The following line is required for outdated versions
    ** of libglew and is introduced as a fix for Ubuntu builds.
    */
    glewExperimental = GL_TRUE;

    glewInit();
    ImGui_ImplGlfwGL3_Init(window, true);
    gui::init(&state);

    xtcore::init();
    xtcore::Log::handle().echo(false);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplGlfwGL3_NewFrame();
        glfwGetFramebufferSize(window, &(state.window.width), &(state.window.height));
        glViewport(0, 0, state.window.width, state.window.height);
        glClearColor(0.20,0.25,0.25,1);
        glClear(GL_COLOR_BUFFER_BIT);
		gui::draw_widgets(&state);
        glfwSwapBuffers(window);
    }
    xtcore::deinit();

    // Cleanup
    ImGui_ImplGlfwGL3_Shutdown();
    glfwTerminate();

    return 0;
}
