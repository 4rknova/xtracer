#include <cstdio>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <nmath/sample.h>
#include <xtcore/math/sampling_util.h>

namespace {

enum sample_mode_t
{
    MODE_SPHERE = 0,
    MODE_HEMISPHERE,
    MODE_DIFFUSE,
    MODE_COSINE_HEMI,
    MODE_POWER_LOBE,
    MODE_COUNT
};

struct vertex_t
{
    float x;
    float y;
    float r;
    float g;
    float b;
};

const char *mode_name(sample_mode_t mode)
{
    switch (mode) {
    case MODE_SPHERE: return "nmath::sample::sphere";
    case MODE_HEMISPHERE: return "nmath::sample::hemisphere";
    case MODE_DIFFUSE: return "nmath::sample::diffuse";
    case MODE_COSINE_HEMI: return "xtcore::math::sampling::sample_cosine_hemisphere";
    case MODE_POWER_LOBE: return "xtcore::math::sampling::sample_power_cosine_lobe";
    default: return "unknown";
    }
}

void print_help(sample_mode_t mode)
{
    std::printf("Sampling visualization\n");
    std::printf("  1 sphere\n");
    std::printf("  2 hemisphere\n");
    std::printf("  3 diffuse\n");
    std::printf("  4 cosine hemisphere\n");
    std::printf("  5 power cosine lobe\n");
    std::printf("  SPACE regenerate points\n");
    std::printf("Current mode: %s\n", mode_name(mode));
}

bool check_shader_compile(GLuint shader)
{
    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok == GL_TRUE) return true;

    GLint len = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
    std::vector<char> log((size_t)len + 1, '\0');
    glGetShaderInfoLog(shader, len, &len, log.data());
    std::fprintf(stderr, "shader compile failed: %s\n", log.data());
    return false;
}

bool check_program_link(GLuint prog)
{
    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (ok == GL_TRUE) return true;

    GLint len = 0;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
    std::vector<char> log((size_t)len + 1, '\0');
    glGetProgramInfoLog(prog, len, &len, log.data());
    std::fprintf(stderr, "program link failed: %s\n", log.data());
    return false;
}

void regenerate(std::vector<vertex_t> &out, sample_mode_t mode, int count)
{
    out.clear();
    out.reserve((size_t)count);

    const nmath::Vector3f up(0.0f, 1.0f, 0.0f);
    const nmath::Vector3f in_dir(0.0f, 1.0f, 0.0f);

    for (int i = 0; i < count; ++i) {
        nmath::Vector3f s;
        nmath::scalar_t pdf = 0.0;
        switch (mode) {
        case MODE_SPHERE:
            s = nmath::sample::sphere();
            break;
        case MODE_HEMISPHERE:
            s = nmath::sample::hemisphere(up, in_dir);
            break;
        case MODE_DIFFUSE:
            s = nmath::sample::diffuse(up);
            break;
        case MODE_COSINE_HEMI:
            s = xtcore::math::sampling::sample_cosine_hemisphere(up, pdf);
            break;
        case MODE_POWER_LOBE:
            s = xtcore::math::sampling::sample_power_cosine_lobe(up, 16.0f, pdf);
            break;
        default:
            s = nmath::sample::sphere();
            break;
        }

        // Visualize in XZ plane (Y drives color).
        const float x = (float)s.x;
        const float y = (float)s.z;
        const float shade = (float)(0.5f * (s.y + 1.0f));

        vertex_t v;
        v.x = x;
        v.y = y;
        v.r = 0.15f + 0.85f * shade;
        v.g = 0.35f + 0.65f * (1.0f - shade);
        v.b = 1.0f - 0.5f * shade;
        out.push_back(v);
    }
}

} // namespace

int main()
{
    const int width = 960;
    const int height = 960;
    const int point_count = 60000;

    if (glfwInit() == GL_FALSE) {
        std::fprintf(stderr, "failed to init glfw\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(width, height, "xtracer sampling viz", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "failed to create window\n");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::fprintf(stderr, "failed to init glew\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    const char *vs_src =
        "#version 330 core\n"
        "layout(location=0) in vec2 a_pos;\n"
        "layout(location=1) in vec3 a_col;\n"
        "out vec3 v_col;\n"
        "void main() {\n"
        "  gl_Position = vec4(a_pos, 0.0, 1.0);\n"
        "  gl_PointSize = 2.0;\n"
        "  v_col = a_col;\n"
        "}\n";

    const char *fs_src =
        "#version 330 core\n"
        "in vec3 v_col;\n"
        "out vec4 frag;\n"
        "void main() {\n"
        "  frag = vec4(v_col, 1.0);\n"
        "}\n";

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vs_src, nullptr);
    glCompileShader(vs);
    if (!check_shader_compile(vs)) return 1;

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fs_src, nullptr);
    glCompileShader(fs);
    if (!check_shader_compile(fs)) return 1;

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    if (!check_program_link(prog)) return 1;

    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void *)(sizeof(float) * 2));
    glEnableVertexAttribArray(1);

    sample_mode_t mode = MODE_SPHERE;
    print_help(mode);

    std::vector<vertex_t> points;
    regenerate(points, mode, point_count);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(points.size() * sizeof(vertex_t)), points.data(), GL_DYNAMIC_DRAW);

    bool last_space = false;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        const bool k1 = glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS;
        const bool k2 = glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS;
        const bool k3 = glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS;
        const bool k4 = glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS;
        const bool k5 = glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS;
        const bool space = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

        sample_mode_t wanted = mode;
        if (k1) wanted = MODE_SPHERE;
        else if (k2) wanted = MODE_HEMISPHERE;
        else if (k3) wanted = MODE_DIFFUSE;
        else if (k4) wanted = MODE_COSINE_HEMI;
        else if (k5) wanted = MODE_POWER_LOBE;

        if (wanted != mode || (space && !last_space)) {
            mode = wanted;
            regenerate(points, mode, point_count);
            glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(points.size() * sizeof(vertex_t)), points.data(), GL_DYNAMIC_DRAW);
            std::printf("mode: %s (%d points)\n", mode_name(mode), point_count);
        }
        last_space = space;

        glViewport(0, 0, width, height);
        glClearColor(0.02f, 0.02f, 0.025f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(prog);
        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, (GLsizei)points.size());

        glfwSwapBuffers(window);
    }

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
