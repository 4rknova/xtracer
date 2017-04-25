#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <list>
#include <queue>
#include <mutex>

#include <GL/glew.h>

#ifndef __APPLE__
#include <GL/glut.h>
#else
#include <GLUT/glut.h>
#endif

#include <xtcore/tile.h>
#include <xtcore/argparse.h>
#include <xtcore/log.h>
#include <xtcore/plr_photonmapper/renderer.h>

#include "shader.h"
#include "gui.h"

static Shader  *postsdr;
static Program *postprg;

static const char *postsdr_source =
	"uniform sampler2D tex;\n"
	"uniform vec2 iWindowResolution;\n"
	"uniform vec2 iRenderResolution;\n"
	"uniform vec2 iTileSize;\n"
	"void main()\n"
	"{\n"
	"    vec3 col = texture2D(tex, gl_TexCoord[0].st).xyz;\n"
    "    vec2 uv = floor(gl_FragCoord.xy / iWindowResolution.xy * (iRenderResolution.xy / iTileSize.xy));\n"
    "    col = vec3(mod(uv.x + uv.y, 2.));\n"
	"    gl_FragColor = vec4(col, 1.0);\n"
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
    mut0.lock();
    xtracer::render::tile_t *t = (xtracer::render::tile_t *)blk;
    printf("Rendering %lux%lu -> %lux%lu \n", t->x0(), t->y0(), t->x1(), t->y1());
    mut0.unlock();
    return 0;
}

static int block_done(void *blk)
{
	mut1.lock();
	dirty_areas.push((xtracer::render::tile_t*)blk);
    xtracer::render::tile_t *t = (xtracer::render::tile_t *)blk;
    printf("Rendered %lux%lu -> %lux%lu \n", t->x0(), t->y0(), t->x1(), t->y1());
	mut1.unlock();
    return 0;
}

void display(void)
{
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

	/*
    // update dirty areas of the output texture
	if (dirty_areas.size() > 0) {
		while(dirty_count > 0) {
			mut0.lock();
			FrameBlock blk = dirty_areas.front();
			dirty_areas.pop();
			dirty_count--;
			mut0.unlock();

			float *pix = ctx.framebuf->pixels + (blk.y * ctx.framebuf->width + blk.x) * 3;

			for(int i=0; i<blk.height; i++) {
				glTexSubImage2D(GL_TEXTURE_2D, 0, blk.x, blk.y + i, blk.width, 1, GL_RGB, GL_FLOAT, pix);
				pix += ctx.framebuf->width * 3
			}
		}
		glutPostRedisplay();
	}
	*/

	glBegin(GL_QUADS);
	glTexCoord2f(0, 1);
	glVertex2f(-1, -1);
	glTexCoord2f(1, 1);
	glVertex2f(1, -1);
	glTexCoord2f(1, 0);
	glVertex2f(1, 1);
	glTexCoord2f(0, 0);
	glVertex2f(-1, 1);
	glEnd();

    glutSwapBuffers();
}

void reshape(int w, int h)
{
    postprg->set_uniform("iWindowResolution", NMath::Vector2f((float)w, (float)h));
    glViewport(0,0,(GLsizei)w,(GLsizei)h);
    glutPostRedisplay();
}

static void keydown(unsigned char key, int x, int y)
{
	if(key == 27) {
		exit(0);
	}
}

int main(int argc, char **argv)
{
    std::list<std::string>modifiers;
    xtracer::render::params_t params;
    xtracer::render::context_t ctx;

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

    xtracer::render::IRenderer *renderer = new Renderer();

    ctx.scene  = &scene;
    ctx.params = params;
    ctx.init();

    renderer->setup(ctx);

    xtracer::render::Tileset::iterator it = ctx.tiles.begin();
    xtracer::render::Tileset::iterator et = ctx.tiles.end();
    for (; it != et; ++it) (*it).setup(block_begin, 0);
    renderer->render();

return 0;

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
    postprg->set_uniform("iTileSize", ts);
    postprg->set_uniform("iRenderResolution", sz);

    if(!postprg->link()) return 1;
	bind_program(postprg);

    glutMainLoop();

	return 0;
}
