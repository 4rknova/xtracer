#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <list>
#include <queue>
#include <mutex>
#include <thread>

#include <GL/glew.h>

#ifndef __APPLE__
#include <GL/glut.h>
#else
#include <GLUT/glut.h>
#endif

#include <nmath/vector.h>
#include <xtcore/tile.h>
#include <xtcore/argparse.h>
#include <xtcore/log.h>
#include <xtcore/plr_photonmapper/renderer.h>

#include "shader.h"
#include "gui.h"

xtracer::render::IRenderer *renderer = 0;
xtracer::render::context_t ctx;

static Shader  *postsdr;
static Program *postprg;
static GLuint   pix;
static const char *postsdr_source =
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

int window = 0;

/* this function is called from the context of the worker thread
 * so it can't call any OpenGL functions directly. Instead it grabs
 * the mutex, shoves he dirty areas into a list, and notifies glut
 * to redisplay.
 */
static std::queue<xtracer::render::tile_t*> dirty_areas;
static std::mutex mut0, mut1;

static int block_begin(void *blk)
{
    return 0;
}

static int block_done(void *blk)
{
	mut0.lock();
	dirty_areas.push((xtracer::render::tile_t *)blk);
	mut0.unlock();
    return 0;
}

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
    // update dirty areas of the output texture
	mut0.lock();
	while(dirty_areas.size() > 0) {
        xtracer::render::tile_t *t = dirty_areas.front();
		dirty_areas.pop();

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

	glBegin(GL_QUADS);
	glTexCoord2f(0, 1);	glVertex2f(-1,-1);
	glTexCoord2f(1, 1);	glVertex2f( 1,-1);
	glTexCoord2f(1, 0);	glVertex2f( 1, 1);
	glTexCoord2f(0, 0);	glVertex2f(-1, 1);
	glEnd();

    glutSwapBuffers();
}

void reshape(int w, int h)
{
    postprg->set_uniform("iWindowResolution", NMath::Vector2f((float)w, (float)h));
    glViewport(0,0,(GLsizei)w,(GLsizei)h);
    glutPostRedisplay();
}

void task_render()
{
    if (renderer) {
        Log::handle().post_message("Starting rendering thread..");
        renderer->render();
    }
}

static void keydown(unsigned char key, int x, int y)
{
    switch (key) {
        case 27 :  exit(0);
        case 'r': case 'R': {
            std::thread t1(task_render);
            t1.detach();
            break;
        }
	}
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
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutInitWindowSize(ctx.params.width, ctx.params.height);

    window = glutCreateWindow(argv[0]);
    glutDisplayFunc(display);
    glutIdleFunc(display);
    glutReshapeFunc(reshape);
	glutKeyboardFunc(keydown);

	glewInit();

    postsdr = new Shader(GL_FRAGMENT_SHADER);
	postprg = new Program;
	postsdr->set_source(postsdr_source);
	postprg->add_shader(postsdr);
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
