#include <cstdio>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <GL/glext.h>
#include <GL/glut.h>
#include <X11/X.h>
#include <cmath>
#include <time.h>

#include <nimg/pixmap.h>

#define STRINGIFY(A) #A

char const *vsh =
#include "demo.vsh"

char const *fsh=
#include "demo.fsh"


#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER   0x8B31
#define glCompileShader      ((PFNGLCOMPILESHADERPROC     ) glXGetProcAddress((const GLubyte*)"glCompileShader"))
#define glAttachShader       ((PFNGLATTACHSHADERPROC      ) glXGetProcAddress((const GLubyte*)"glAttachShader"))
#define glCreateProgram      ((PFNGLCREATEPROGRAMPROC     ) glXGetProcAddress((const GLubyte*)"glCreateProgram"))
#define glCreateShader       ((PFNGLCREATESHADERPROC      ) glXGetProcAddress((const GLubyte*)"glCreateShader"))
#define glShaderSource       ((PFNGLSHADERSOURCEPROC      ) glXGetProcAddress((const GLubyte*)"glShaderSource"))
#define glLinkProgram        ((PFNGLLINKPROGRAMPROC       ) glXGetProcAddress((const GLubyte*)"glLinkProgram"))
#define glUseProgram         ((PFNGLUSEPROGRAMPROC        ) glXGetProcAddress((const GLubyte*)"glUseProgram"))
#define glUniform1i          ((PFNGLUNIFORM1IPROC         ) glXGetProcAddress((const GLubyte*)"glUniform1i"))
#define glUniform2f          ((PFNGLUNIFORM2FPROC         ) glXGetProcAddress((const GLubyte*)"glUniform2f"))
#define glGetUniformLocation ((PFNGLGETUNIFORMLOCATIONPROC) glXGetProcAddress((const GLubyte*)"glGetUniformLocation"))

#define WIDTH  800
#define HEIGHT 600

GLuint tex = 0, p = 0;

long ms()
{
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds
}

long t = ms();

void display(void)
{
	/*
    surface_t temp = front;
    front = back;
    back = temp;

    float s = float(t-ms())/1000. * M_PI * 2.;
    clear(back, {0,0,0});
    tri.b.v.y = 100 + int(cos(s) * 250.f);
//    draw_triangle(back, tri);
//    fill_triangle(back, tri);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, back.);
*/
    glClearColor(1, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glRecti(1,1,-1,-1);
    glutSwapBuffers();
    glutPostRedisplay();
}

void reshape(int w, int h)
{
    glViewport(0,0,(GLsizei)w,(GLsizei)h);
    glUniform2f(glGetUniformLocation(p, "iResolution"), w, h);
    glUniform1i(glGetUniformLocation(p, "iChannel0"), 0);
}


void init()
{
}

void cleanup()
{
}

int gl_main(int argc, char **argv)
{
    init();
    putenv( (char *) "__GL_SYNC_TO_VBLANK=1" );
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Rasterizer");
    glutIdleFunc(display);
    glutReshapeFunc(reshape);

    GLuint s = 0;
    p = glCreateProgram();
    s = glCreateShader(GL_VERTEX_SHADER);   glShaderSource(s, 1, &vsh, 0); glCompileShader(s); glAttachShader(p,s);
    s = glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(s, 1, &fsh, 0); glCompileShader(s); glAttachShader(p,s);
    glLinkProgram(p);
    glUseProgram(p);

    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glutMainLoop();

    cleanup();

    return 0;
}
