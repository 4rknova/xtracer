#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <list>
#include <queue>
#include <mutex>
#include <thread>
#include <stdint.h>

#include <nmath/vector.h>
#include <xtcore/tile.h>
#include <xtcore/argparse.h>
#include <xtcore/log.h>
#include <xtcore/plr_photonmapper/renderer.h>
#include "ext/stb_image.h"
#include "widgets.h"
#include "opengl.h"
#include "shader.h"
#include "logo.h"

#include "state.h"

gui::state_t state;

static Shader  *postsdr_frag;
static Program *postprg;

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

static std::queue<xtracer::render::tile_t*> tiles_done;
static std::queue<xtracer::render::tile_t*> tiles_started;
static std::mutex mut0, mut1;

GLuint load_logo()
{
    int w, h, comp;
    unsigned char* image = stbi_load_from_memory(res_logo_png, res_logo_png_len, &w, &h, &comp, STBI_rgb_alpha);
    glGenTextures(1, &(state.textures.logo));
    glBindTexture(GL_TEXTURE_2D, state.textures.logo);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    stbi_image_free(image);

    return 1;
}

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

void task_render()
{
    if (state.renderer && !(state.flags.rendering)) {
        state.flags.rendering = true;
        state.renderer->render();
        state.flags.rendering = false;
    }
}

void render() {
    std::thread t1(task_render);
    t1.detach();
}

void display(void)
{
    mut0.lock();
	while(tiles_started.size() > 0) {
        xtracer::render::tile_t *t = tiles_started.front();
		tiles_started.pop();

		for (int y = t->y0(); y < t->y1(); ++y) {
            for (int x = t->x0(); x < t->x1(); ++x) {
                nimg::ColorRGBf col;
                t->read(x, y, col);
                float data[4] = {1,0,0,1};
                glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, 1, 1, GL_RGBA, GL_FLOAT, data);
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
                float data[4] = {col.r(), col.g(), col.b(), 1};
                glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, 1, 1, GL_RGBA, GL_FLOAT, data);
            }
		}
	}
    mut0.unlock();

    glClearColor(0.02,0.02,0.02,1);
    glClear(GL_COLOR_BUFFER_BIT);

	bind_program(postprg);
	glBegin(GL_QUADS);
	glVertex2f(-1,-1);
	glVertex2f( 1,-1);
	glVertex2f( 1, 1);
	glVertex2f(-1, 1);
	glEnd();

    gui::draw_widgets(&state);

    gui::ACTION action = gui::get_action();
    switch (action) {
        case gui::ACTION_RENDER: render(); break;
    }

    glutSwapBuffers();
}

void reshape(int w, int h)
{
	state.window.width  = w;
	state.window.height = h;
    postprg->set_uniform("iWindowResolution", NMath::Vector2f((float)w, (float)h));
    glViewport(0,0,(GLsizei)w,(GLsizei)h);
    glutPostRedisplay();
}

static void keydown(unsigned char key, int x, int y)
{
    gui::handle_io_kb(key);
    switch (key) { case 27 :  exit(0); }
}


bool mouseEvent(int button, int state, int x, int y)
{
    bool l = (state == GLUT_DOWN && (button == GLUT_LEFT_BUTTON ));
    bool r = (state == GLUT_DOWN && (button == GLUT_RIGHT_BUTTON));

    float wheel = 0.f;

    if (state == GLUT_DOWN && button == 3) wheel = 1.f;
    if (state == GLUT_DOWN && button == 4) wheel =-1.f;

    gui::handle_io_ms(x, y, true, l, r, wheel);

    return true;
}

void mouseCallback(int button, int state, int x, int y)
{
    if (mouseEvent(button, state, x, y))glutPostRedisplay();
}

void mouseDragCallback(int x, int y)
{
    gui::handle_io_ms(x, y);
    glutPostRedisplay();
}

void mouseMoveCallback(int x, int y)
{
    gui::handle_io_ms(x, y);
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

    state.renderer = new Renderer();

    state.ctx.scene  = &scene;
    state.ctx.params = params;
    state.ctx.init();

    state.renderer->setup(state.ctx);

    xtracer::render::Tileset::iterator it = state.ctx.tiles.begin();
    xtracer::render::Tileset::iterator et = state.ctx.tiles.end();
    for (; it != et; ++it) (*it).setup(block_begin, block_done);

    int zero = 0;
    glutInit(&zero, 0);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(WINDOW_DEFAULT_WIDTH, WINDOW_DEFAULT_HEIGHT);

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

    gui::init(&state);

    postsdr_frag = new Shader(GL_FRAGMENT_SHADER);
	postprg = new Program;
	postsdr_frag->set_source(postsdr_source_frag);
	postprg->add_shader(postsdr_frag);
    NMath::Vector2f ts((float)(state.ctx.params.tile_size), (float)(state.ctx.params.tile_size));
    NMath::Vector2f sz((float)(state.ctx.params.width    ), (float)(state.ctx.params.height   ));

    if(!postprg->link()) return 1;

    state.textures.logo = load_logo();

    glGenTextures(1, &(state.textures.render));
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, state.textures.render);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    float *data = new float[state.ctx.params.width * state.ctx.params.height*4];
    memset(data, 0, sizeof(float) * 4 * state.ctx.params.width * state.ctx.params.height);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, state.ctx.params.width, state.ctx.params.height, 0, GL_RGBA, GL_FLOAT, data);
    delete data;

    glutMainLoop();

	return 0;
}
