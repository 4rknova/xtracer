#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <queue>
#include <mutex>

#include <GL/glew.h>

#ifndef __APPLE__
#include <GL/glut.h>
#else
#include <GLUT/glut.h>
#endif

#include <xtcore/tile.h>

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
static std::queue<xtracer::render::tile_t> dirty_areas;
static std::mutex dirty_mutex;

static void block_done(const xtracer::render::tile_t &blk)
{
	dirty_mutex.lock();
	dirty_areas.push(blk);
	dirty_mutex.unlock();
}

void display(void)
{
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

	/*
    // update dirty areas of the output texture
	if (dirty_areas.size() > 0) {
		while(dirty_count > 0) {
			dirty_mutex.lock();
			FrameBlock blk = dirty_areas.front();
			dirty_areas.pop();
			dirty_count--;
			dirty_mutex.unlock();

			float *pix = ctx.framebuf->pixels + (blk.y * ctx.framebuf->width + blk.x) * 3;

			for(int i=0; i<blk.height; i++) {
				glTexSubImage2D(GL_TEXTURE_2D, 0, blk.x, blk.y + i, blk.width, 1, GL_RGB, GL_FLOAT, pix);
				pix += ctx.framebuf->width * 3;
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

int gui(xtracer::render::context_t &ctx, xtracer::render::IRenderer *renderer)
{
    // Initialization stuff
    int argc = 0;
    glutInit(&argc, 0);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutInitWindowSize(ctx.params.width, ctx.params.height);

    // Create  window main
    window = glutCreateWindow("xtracer");
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

    // Enter Glut Main Loop and wait for events
    glutMainLoop();
	return 0;
}

int main(int argc, char **argv)
{
    xtracer::render::context_t ctx;
    xtracer::render::IRenderer *renderer = 0;

    gui(ctx, renderer);
}
