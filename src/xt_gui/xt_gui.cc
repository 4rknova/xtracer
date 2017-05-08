#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <list>
#include <queue>
#include <mutex>
#include <thread>

#include <nmath/vector.h>
#include <xtcore/tile.h>
#include <xtcore/argparse.h>
#include <xtcore/log.h>
#include <xtcore/plr_photonmapper/renderer.h>

#include "ext/imgui.h"
#include "ext/imgui_impl_glut.h"

#include "opengl.h"
#include "shader.h"

xtracer::render::IRenderer *renderer = 0;
xtracer::render::context_t ctx;

static Shader  *postsdr_frag;
static Program *postprg;
static GLuint   pix;
bool rendering = false;

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
    "    uv.y = 1. - uv.y;\n"
	"    vec3 cl = texture(tex, uv).xyz;\n"
    "    Color = vec4(cl,1);\n"
	"}\n";

static std::queue<xtracer::render::tile_t*> tiles_done;
static std::queue<xtracer::render::tile_t*> tiles_started;
static std::mutex mut0, mut1;

unsigned int width;
unsigned int height;
bool show_test_window = true;
bool show_another_window = false;

static int block_begin(void *blk)
{
	mut0.lock();
	tiles_started.push((xtracer::render::tile_t *)blk);
	mut0.unlock();
    return 0;
}

static int block_done(void *blk)
{
	mut0.lock();
	tiles_done.push((xtracer::render::tile_t *)blk);
	mut0.unlock();
    return 0;
}

void draw_widgets()
{
	ImGui_ImplGLUT_NewFrame(width, height);

    // 1. Show a simple window
    // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
    {
        static float f = 0.0f;
        ImGui::Text("Hello, world!");
        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        if (ImGui::Button("Test Window")) show_test_window ^= 1;
        if (ImGui::Button("Another Window")) show_another_window ^= 1;
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    }

    // 2. Show another simple window, this time using an explicit Begin/End pair
    if (show_another_window)
    {
        ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
        ImGui::Begin("Another Window", &show_another_window);
        ImGui::Text("Hello");
        ImGui::End();
    }

    // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
    if (show_test_window)
    {
        ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
        ImGui::ShowTestWindow(&show_test_window);
    }

    ImGui::Render();
}

void display(void)
{
    glClearColor(1,1,1,1);
    glClear(GL_COLOR_BUFFER_BIT);

    mut0.lock();
	while(tiles_started.size() > 0) {
        xtracer::render::tile_t *t = tiles_started.front();
		tiles_started.pop();

		for (int y = t->y0(); y < t->y1(); ++y) {
            for (int x = t->x0(); x < t->x1(); ++x) {
                nimg::ColorRGBf col;
                t->read(x, y, col);

                float data[3] = {0,0,0};
                if (
                    (x == t->x0()) || (x == t->x1()-1)
                 || (y == t->y0()) || (y == t->y1()-1)
                 || ((x - t->x0()) == (y - t->y0()))
                 || ((x - t->x0()) == (t->y1() - y))
                ) {
                    data[0] = 1;
                }
                glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, 1, 1, GL_RGB, GL_FLOAT, data);
            }
		}
	}

    while(tiles_done.size() > 0) {
        xtracer::render::tile_t *t = tiles_done.front();
		tiles_done.pop();

		for (int y = t->y0(); y < t->y1(); ++y) {
            for (int x = t->x0(); x < t->x1(); ++x) {
                nimg::ColorRGBf col;
                t->read(x, y, col);
                float data[3] = {col.r(), col.g(), col.b()};
                glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, 1, 1, GL_RGB, GL_FLOAT, data);
            }
		}
	}
    mut0.unlock();


	bind_program(postprg);
	glBegin(GL_QUADS);
	glVertex2f(-1,-1);
	glVertex2f( 1,-1);
	glVertex2f( 1, 1);
	glVertex2f(-1, 1);
	glEnd();

    draw_widgets();

    glutSwapBuffers();
}

void reshape(int w, int h)
{
	width  = w;
	height = h;
    postprg->set_uniform("iWindowResolution", NMath::Vector2f((float)w, (float)h));
    glViewport(0,0,(GLsizei)w,(GLsizei)h);
    glutPostRedisplay();
}

void task_render()
{
    if (renderer && !rendering) {
        rendering = true;
        renderer->render();
        rendering = false;
    }
}

void render() {
    std::thread t1(task_render);
    t1.detach();
}

static void keydown(unsigned char key, int x, int y)
{
    xtracer::assets::ICamera *cam = ctx.scene->get_camera();

	ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharacter(key);

    switch (key) {
        case 27 :  exit(0);
        case 'r': case 'R': { render();                        break; }
/*
        case 'w': case 'W': { cam->position.z += 1.; render(); break; }
        case 's': case 'S': { cam->position.z -= 1.; render(); break; }
        case 'a': case 'A': { cam->position.x -= 1.; render(); break; }
        case 'd': case 'D': { cam->position.x += 1.; render(); break; }
        case 'q': case 'Q': { cam->position.y += 1.; render(); break; }
        case 'e': case 'E': { cam->position.y -= 1.; render(); break; }
*/
	}
}


bool mouseEvent(int button, int state, int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)x, (float)y);

    if (state == GLUT_DOWN && (button == GLUT_LEFT_BUTTON))
        io.MouseDown[0] = true;
    else
        io.MouseDown[0] = false;

    if (state == GLUT_DOWN && (button == GLUT_RIGHT_BUTTON))
        io.MouseDown[1] = true;
    else
        io.MouseDown[1] = false;

    // Wheel reports as button 3(scroll up) and button 4(scroll down)
    if (state == GLUT_DOWN && button == 3) // It's a wheel event
        io.MouseWheel = 1.0;
    else
        io.MouseWheel = 0.0;

    if (state == GLUT_DOWN && button == 4) // It's a wheel event
        io.MouseWheel = -1.0;
    else
        io.MouseWheel = 0.0;


    return true;
}

void mouseCallback(int button, int state, int x, int y)
{
    if (mouseEvent(button, state, x, y))
    {
        glutPostRedisplay();
    }
}

void mouseDragCallback(int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)x, (float)y);

    glutPostRedisplay();
}

void mouseMoveCallback(int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)x, (float)y);

    glutPostRedisplay();
}

int main(int argc, char **argv)
{
    std::list<std::string>modifiers;
    xtracer::render::params_t params;

    std::string renderer_name, outdir, scene_path, camera;

    if (setup(argc, argv
        , renderer_name
        , outdir
        , scene_path
        , modifiers
        , camera
        , params
    )) return 1;

    Scene scene;
    if (!scene.load(scene_path.c_str(), modifiers)) {
        if (scene.m_cameras.size() == 0) {
            Log::handle().post_error("no cameras found");
            return 2;
        }
        scene.camera = camera;
    } else return 1;

    renderer = new Renderer();

    ctx.scene  = &scene;
    ctx.params = params;
    ctx.init();

    renderer->setup(ctx);

    xtracer::render::Tileset::iterator it = ctx.tiles.begin();
    xtracer::render::Tileset::iterator et = ctx.tiles.end();
    for (; it != et; ++it) (*it).setup(block_begin, block_done);

    int zero = 0;
    glutInit(&zero, 0);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(ctx.params.width, ctx.params.height);

    int window = glutCreateWindow(argv[0]);
    glutDisplayFunc(display);
    glutIdleFunc(display);
    glutReshapeFunc(reshape);
	glutKeyboardFunc(keydown);
    glutMouseFunc(mouseCallback);
    glutMotionFunc(mouseDragCallback);
    glutPassiveMotionFunc(mouseMoveCallback);

	glewInit();
    glEnable(GL_MULTISAMPLE);
    glClearColor(0.447f, 0.565f, 0.604f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

	ImGui_ImplGLUT_Init();

    postsdr_frag = new Shader(GL_FRAGMENT_SHADER);
	postprg = new Program;
	postsdr_frag->set_source(postsdr_source_frag);
	postprg->add_shader(postsdr_frag);
    NMath::Vector2f ts((float)(ctx.params.tile_size), (float)(ctx.params.tile_size));
    NMath::Vector2f sz((float)(ctx.params.width    ), (float)(ctx.params.height   ));

    if(!postprg->link()) return 1;
	bind_program(postprg);

    glGenTextures(1, &pix);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, pix);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    float *data = new float[ctx.params.width*ctx.params.height*3];
    memset(data, 0, sizeof(float) * 3 * ctx.params.width * ctx.params.height);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ctx.params.width, ctx.params.height, 0, GL_RGB, GL_FLOAT, data);
    delete data;

    postprg->set_uniform("iTileSize", ts);
    postprg->set_uniform("iRenderResolution", sz);
    postprg->set_uniform("tex", 0);

    glutMainLoop();

	return 0;
}
