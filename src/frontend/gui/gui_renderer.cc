#include <cmath>
#include <vector>

#include <xtcore/log.h>
#include <xtcore/mesh.h>
#include "workspace.h"
#include "gui_renderer.h"

namespace {

#if defined(XTRACER_ENABLE_REALTIME_GL) && XTRACER_ENABLE_REALTIME_GL

GLuint compile_shader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_TRUE) return shader;

    GLint len = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
    if (len > 0) {
        char *buf = new char[len + 1];
        glGetShaderInfoLog(shader, len, nullptr, buf);
        buf[len] = 0;
        xtcore::Log::handle().post_error("realtime_gl shader compile failed: %s", buf);
        delete[] buf;
    }

    glDeleteShader(shader);
    return 0;
}

GLuint create_program()
{
    static const char *vs = R"GLSL(
#version 150 core
in vec3 a_pos;
in vec3 a_nrm;
out vec3 v_nrm;
uniform mat4 u_viewproj;
void main()
{
    v_nrm = a_nrm;
    gl_Position = u_viewproj * vec4(a_pos, 1.0);
}
)GLSL";

    static const char *fs = R"GLSL(
#version 150 core
in vec3 v_nrm;
out vec4 frag_color;
void main()
{
    vec3 n = normalize(v_nrm);
    vec3 l = normalize(vec3(0.45, 0.80, 0.35));
    float ndotl = max(dot(n, l), 0.0);
    vec3 base = vec3(0.22, 0.36, 0.70);
    vec3 col = base * (0.20 + 0.80 * ndotl) + 0.15 * abs(n);
    col = pow(max(col, vec3(0.0)), vec3(1.0 / 2.2));
    frag_color = vec4(col, 1.0);
}
)GLSL";

    GLuint v = compile_shader(GL_VERTEX_SHADER, vs);
    GLuint f = compile_shader(GL_FRAGMENT_SHADER, fs);
    if (!v || !f) {
        if (v) glDeleteShader(v);
        if (f) glDeleteShader(f);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, v);
    glAttachShader(program, f);
    glBindAttribLocation(program, 0, "a_pos");
    glBindAttribLocation(program, 1, "a_nrm");
    glLinkProgram(program);
    glDeleteShader(v);
    glDeleteShader(f);

    GLint status = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_TRUE) return program;

    GLint len = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
    if (len > 0) {
        char *buf = new char[len + 1];
        glGetProgramInfoLog(program, len, nullptr, buf);
        buf[len] = 0;
        xtcore::Log::handle().post_error("realtime_gl program link failed: %s", buf);
        delete[] buf;
    }

    glDeleteProgram(program);
    return 0;
}

void mat4_mul(const float a[16], const float b[16], float out[16])
{
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            out[r * 4 + c] =
                a[r * 4 + 0] * b[0 * 4 + c] +
                a[r * 4 + 1] * b[1 * 4 + c] +
                a[r * 4 + 2] * b[2 * 4 + c] +
                a[r * 4 + 3] * b[3 * 4 + c];
        }
    }
}

void make_view_matrix(const nmath::Vector3f &eye,
                      const nmath::Vector3f &target,
                      const nmath::Vector3f &up,
                      float out[16])
{
    nmath::Vector3f rz = (target - eye).normalized();
    nmath::Vector3f rx = cross(up, rz).normalized();
    nmath::Vector3f ry = cross(rz, rx).normalized();

    out[0] = rx.x; out[1] = rx.y; out[2] = rx.z; out[3] = -dot(rx, eye);
    out[4] = ry.x; out[5] = ry.y; out[6] = ry.z; out[7] = -dot(ry, eye);
    out[8] = rz.x; out[9] = rz.y; out[10] = rz.z; out[11] = -dot(rz, eye);
    out[12] = 0.f; out[13] = 0.f; out[14] = 0.f; out[15] = 1.f;
}

void make_projection_matrix(float fov_degrees, float aspect, float z_near, float z_far, float out[16])
{
    const float f = 1.0f / std::tan((fov_degrees * 0.017453292519943f) * 0.5f);
    const float a = (z_far + z_near) / (z_far - z_near);
    const float b = (-2.0f * z_far * z_near) / (z_far - z_near);

    out[0] = f / aspect; out[1] = 0.f; out[2] = 0.f; out[3] = 0.f;
    out[4] = 0.f; out[5] = f; out[6] = 0.f; out[7] = 0.f;
    out[8] = 0.f; out[9] = 0.f; out[10] = a; out[11] = b;
    out[12] = 0.f; out[13] = 0.f; out[14] = 1.f; out[15] = 0.f;
}

bool build_scene_vertex_buffer(xtcore::Scene &scene, std::vector<float> &vertices)
{
    vertices.clear();

    for (auto it = scene.m_objects.begin(); it != scene.m_objects.end(); ++it) {
        HASH_ID obj_id = (*it).first;
        const xtcore::asset::ISurface *surface = scene.get_surface(obj_id);
        const xtcore::surface::Mesh *mesh = dynamic_cast<const xtcore::surface::Mesh*>(surface);
        if (!mesh) continue;

        const std::vector<xtcore::surface::Triangle> &triangles = mesh->triangles();
        for (size_t t = 0; t < triangles.size(); ++t) {
            const xtcore::surface::Triangle &tri = triangles[t];
            const nmath::Vector3f tri_normal = tri.calc_normal();
            for (int i = 0; i < 3; ++i) {
                const nmath::Vector3f &p = tri.v[i];
                nmath::Vector3f n = tri.n[i];
                if (n.length_squared() == 0.0f) n = tri_normal;
                n.normalize();

                vertices.push_back(p.x);
                vertices.push_back(p.y);
                vertices.push_back(p.z);
                vertices.push_back(n.x);
                vertices.push_back(n.y);
                vertices.push_back(n.z);
            }
        }
    }

    return !vertices.empty();
}

class RealtimeGlRenderer : public gui::IGuiRenderer
{
    public:
    RealtimeGlRenderer()
        : m_fbo(0)
        , m_depth_rbo(0)
        , m_vao(0)
        , m_vbo(0)
        , m_program(0)
        , m_u_viewproj(-1)
        , m_vertex_count(0)
    {}

    ~RealtimeGlRenderer()
    {
        if (m_program) glDeleteProgram(m_program);
        if (m_vbo) glDeleteBuffers(1, &m_vbo);
        if (m_vao) glDeleteVertexArrays(1, &m_vao);
        if (m_depth_rbo) glDeleteRenderbuffers(1, &m_depth_rbo);
        if (m_fbo) glDeleteFramebuffers(1, &m_fbo);
    }

    bool begin(::workspace_t *ws) override
    {
        if (!ws) return false;
        if (!m_program) {
            m_program = create_program();
            if (!m_program) return false;
            m_u_viewproj = glGetUniformLocation(m_program, "u_viewproj");
        }

        if (!m_vao) glGenVertexArrays(1, &m_vao);
        if (!m_vbo) glGenBuffers(1, &m_vbo);
        if (!m_fbo) glGenFramebuffers(1, &m_fbo);
        if (!m_depth_rbo) glGenRenderbuffers(1, &m_depth_rbo);

        std::vector<float> vertices;
        if (!build_scene_vertex_buffer(ws->context.scene, vertices)) {
            xtcore::Log::handle().post_warning("realtime_gl: scene has no mesh triangles to rasterize");
            m_vertex_count = 0;
            return true;
        }

        m_vertex_count = vertices.size() / 6;
        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)(sizeof(float) * 3));
        glBindVertexArray(0);
        return true;
    }

    void render_frame(::workspace_t *ws) override
    {
        if (!ws || !m_program || !m_vao || !m_fbo || !m_depth_rbo) return;
        if (!m_vertex_count) return;

        xtcore::asset::ICamera *cam_base = ws->context.scene.get_camera(ws->context.params.camera);
        xtcore::camera::Perspective *cam = dynamic_cast<xtcore::camera::Perspective*>(cam_base);
        if (!cam) {
            xtcore::Log::handle().post_warning("realtime_gl rasterization currently supports Perspective camera only");
            return;
        }

        const float w = (float)ws->context.params.width;
        const float h = (float)ws->context.params.height;
        const float aspect = w / (h > 0.f ? h : 1.f);

        float view[16], proj[16], viewproj[16];
        make_view_matrix(cam->position, cam->target, cam->up, view);
        make_projection_matrix(cam->fov, aspect, 0.01f, 1000.0f, proj);
        mat4_mul(proj, view, viewproj);

        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ws->texture, 0);

        glBindRenderbuffer(GL_RENDERBUFFER, m_depth_rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, ws->context.params.width, ws->context.params.height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth_rbo);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            xtcore::Log::handle().post_error("realtime_gl framebuffer is incomplete");
            return;
        }

        glViewport(0, 0, ws->context.params.width, ws->context.params.height);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.06f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(m_program);
        glUniformMatrix4fv(m_u_viewproj, 1, GL_TRUE, viewproj);
        glBindVertexArray(m_vao);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_vertex_count);
        glBindVertexArray(0);
        glUseProgram(0);

        glDisable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void end(::workspace_t *) override {}

    private:
    GLuint m_fbo;
    GLuint m_depth_rbo;
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_program;
    GLint  m_u_viewproj;
    size_t m_vertex_count;
};

#endif

} // namespace

namespace gui {

IGuiRenderer *create_gui_renderer(workspace_t *ws)
{
#if defined(XTRACER_ENABLE_REALTIME_GL) && XTRACER_ENABLE_REALTIME_GL
    if (ws && ws->uses_realtime_gl()) return new RealtimeGlRenderer();
#else
    (void)ws;
#endif
    return nullptr;
}

void destroy_gui_renderer(IGuiRenderer *renderer, workspace_t *ws)
{
    if (!renderer) return;
    renderer->end(ws);
    delete renderer;
}

} // namespace gui
