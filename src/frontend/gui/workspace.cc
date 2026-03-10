#include <cmath>
#include <vector>

#include <xtcore/memutil.tml>
#include <xtcore/parseutil.h>
#include <xtcore/log.h>
#include <xtcore/mesh.h>
#include <nimg/conversion.h>
#include "workspace.h"

namespace {

bool is_realtime_gl_integrator(const xtcore::render::IIntegrator *integrator)
{
#if defined(XTRACER_ENABLE_REALTIME_GL) && XTRACER_ENABLE_REALTIME_GL
    return dynamic_cast<const xtcore::integrator::realtime_gl::Integrator*>(integrator) != nullptr;
#else
    (void)integrator;
    return false;
#endif
}

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
#endif

} // namespace

ws_handler_t::ws_handler_t(std::mutex *m)
    : mut(m)
{}

void ws_handler_t::handle_event(xtcore::render::tile_t *tile)
{
	mut->lock();
	tiles.push(tile);
    mut->unlock();
}

xtcore::render::tile_t *ws_handler_t::pop()
{
    xtcore::render::tile_t *t = nullptr;

    if (tiles.size() > 0) {
        t = tiles.front();
        tiles.pop();
    }

    return t;
}

void workspace_t::load()
{
    int err = xtcore::io::scn::load(&(context.scene), source_file.c_str());
    status = (err ? WS_STATUS_INVALID : WS_STATUS_LOADED);

    // Auto-select camera
    {
        xtcore::render::params_t *p = &(context.params);
        auto first_cam = context.scene.m_cameras.begin();
        bool is_cam_valid = (first_cam != context.scene.m_cameras.end());
        if (is_cam_valid && (p->camera == HASH_ID_INVALID)) p->camera = (*first_cam).first;
    }

    gui::graph::build(&graph, &(context.scene));
}

void workspace_t::prepare()
{
    const size_t pixel_count = context.params.width * context.params.height;
    float *data = nullptr;
    if (clear_buffer) {
        data = new float[pixel_count * 4];
        memset(data, 0, sizeof(float) * 4 * pixel_count);
    }
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, context.params.width, context.params.height, 0, GL_RGBA, GL_FLOAT, data);
    if (data) delete[] data;
}

void workspace_t::render()
{
    status   = WS_STATUS_PROCESSING;
    progress = 0.f;

    context.init();
    setup_callbacks();
    integrator->setup(context);
    xtcore::render::order(context.tiles, context.params.tile_order);

    switch (rmode) {
        case WS_RMODE_SINGLE:
        {
            timer.start();
            integrator->render();
            timer.stop();
        } break;
        case WS_RMODE_CONTINUOUS:
        {
            while (rmode == WS_RMODE_CONTINUOUS) {
                integrator->render();
            }
        } break;
    }

    status = WS_STATUS_LOADED;
}

void workspace_t::setup_callbacks()
{
    progress = 0.f;
    for (auto& i : context.tiles) {
        i.setup_handler_on_init(&handler_init);
        i.setup_handler_on_done(&handler_done);
    }
}

bool workspace_t::is_idle()
{
    return status == WS_STATUS_LOADED;
}

bool workspace_t::is_rendering()
{
    return status == WS_STATUS_PROCESSING || realtime_gl_active;
}

void workspace_t::update()
{
    if (realtime_gl_active) {
        render_realtime_gl_frame();
        return;
    }

    float pu = 1.f/context.tiles.size();

    m.lock();

    glBindTexture(GL_TEXTURE_2D, texture);

    if (show_tile_updates) {
    	while (1) {
            xtcore::render::tile_t *t = handler_init.pop();

            if (!t) break;

    		for (size_t y = t->y0(); y < t->y1(); ++y) {
                for (size_t x = t->x0(); x < t->x1(); ++x) {
                    float data[4] = {1,0,0,1};
                    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, 1, 1, GL_RGBA, GL_FLOAT, data);
                }
    		}
    	}
    }

    while (1) {
        xtcore::render::tile_t *t = handler_done.pop();

        if (!t) { break; }

		for (size_t y = t->y0(); y < t->y1(); ++y) {
            for (size_t x = t->x0(); x < t->x1(); ++x) {
                nimg::ColorRGBAf col;
                t->read(x, y, col);
                float data[4] = {
                    linear_to_srgb(col.r()),
                    linear_to_srgb(col.g()),
                    linear_to_srgb(col.b()),
                    col.a()
                };
                glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, 1, 1, GL_RGBA, GL_FLOAT, data);
            }
		}
        progress += pu;
	}
    m.unlock();
}

bool workspace_t::uses_realtime_gl() const
{
    return is_realtime_gl_integrator(integrator);
}

void workspace_t::stop_realtime_gl()
{
    realtime_gl_active = false;
}

void workspace_t::start_realtime_gl()
{
#if !(defined(XTRACER_ENABLE_REALTIME_GL) && XTRACER_ENABLE_REALTIME_GL)
    return;
#else
    if (!uses_realtime_gl()) return;

    context.init();
    status = WS_STATUS_LOADED;
    progress = 0.f;
    timer.start();

    if (!realtime_gl_program) {
        realtime_gl_program = create_program();
        if (!realtime_gl_program) {
            xtcore::Log::handle().post_error("realtime_gl failed to initialize shader program");
            return;
        }
        realtime_gl_u_viewproj = glGetUniformLocation(realtime_gl_program, "u_viewproj");
    }

    if (!realtime_gl_vao) glGenVertexArrays(1, &realtime_gl_vao);
    if (!realtime_gl_vbo) glGenBuffers(1, &realtime_gl_vbo);
    if (!realtime_gl_fbo) glGenFramebuffers(1, &realtime_gl_fbo);
    if (!realtime_gl_depth_rbo) glGenRenderbuffers(1, &realtime_gl_depth_rbo);

    std::vector<float> vertices;
    if (!build_scene_vertex_buffer(context.scene, vertices)) {
        xtcore::Log::handle().post_warning("realtime_gl: scene has no mesh triangles to rasterize");
        realtime_gl_vertex_count = 0;
    } else {
        realtime_gl_vertex_count = vertices.size() / 6;
        glBindVertexArray(realtime_gl_vao);
        glBindBuffer(GL_ARRAY_BUFFER, realtime_gl_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)(sizeof(float) * 3));
        glBindVertexArray(0);
    }

    if (rmode == WS_RMODE_SINGLE) {
        render_realtime_gl_frame();
        timer.stop();
        progress = 1.f;
        realtime_gl_active = false;
    } else {
        realtime_gl_active = true;
    }
#endif
}

void workspace_t::render_realtime_gl_frame()
{
#if !(defined(XTRACER_ENABLE_REALTIME_GL) && XTRACER_ENABLE_REALTIME_GL)
    return;
#else
    if (!realtime_gl_program || !realtime_gl_vao || !realtime_gl_fbo || !realtime_gl_depth_rbo) return;
    if (!realtime_gl_vertex_count) return;

    xtcore::asset::ICamera *cam_base = context.scene.get_camera(context.params.camera);
    xtcore::camera::Perspective *cam = dynamic_cast<xtcore::camera::Perspective*>(cam_base);
    if (!cam) {
        xtcore::Log::handle().post_warning("realtime_gl rasterization currently supports Perspective camera only");
        realtime_gl_active = false;
        return;
    }

    const float w = (float)context.params.width;
    const float h = (float)context.params.height;
    const float aspect = w / (h > 0.f ? h : 1.f);

    float view[16], proj[16], viewproj[16];
    make_view_matrix(cam->position, cam->target, cam->up, view);
    make_projection_matrix(cam->fov, aspect, 0.01f, 1000.0f, proj);
    mat4_mul(proj, view, viewproj);

    glBindFramebuffer(GL_FRAMEBUFFER, realtime_gl_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, realtime_gl_depth_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, context.params.width, context.params.height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, realtime_gl_depth_rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        xtcore::Log::handle().post_error("realtime_gl framebuffer is incomplete");
        realtime_gl_active = false;
        return;
    }

    glViewport(0, 0, context.params.width, context.params.height);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.06f, 0.08f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(realtime_gl_program);
    glUniformMatrix4fv(realtime_gl_u_viewproj, 1, GL_TRUE, viewproj);
    glBindVertexArray(realtime_gl_vao);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)realtime_gl_vertex_count);
    glBindVertexArray(0);
    glUseProgram(0);

    glDisable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    progress = 1.f;
#endif
}

workspace_t::workspace_t()
    : status(WS_STATUS_INVALID)
    , texture(0)
    , zoom_multiplier(1.001f)
    , integrator(0)
    , handler_init(&m)
    , handler_done(&m)
    , gamma(DEFAULT_GAMMA)
    , clear_buffer(true)
    , show_tile_updates(true)
    , rmode(WS_RMODE_SINGLE)
    , realtime_gl_active(false)
    , realtime_gl_fbo(0)
    , realtime_gl_depth_rbo(0)
    , realtime_gl_vao(0)
    , realtime_gl_vbo(0)
    , realtime_gl_program(0)
    , realtime_gl_u_viewproj(-1)
    , realtime_gl_vertex_count(0)
{
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S    , GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T    , GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
}

workspace_t::~workspace_t()
{
    if (realtime_gl_program) glDeleteProgram(realtime_gl_program);
    if (realtime_gl_vbo) glDeleteBuffers(1, &realtime_gl_vbo);
    if (realtime_gl_vao) glDeleteVertexArrays(1, &realtime_gl_vao);
    if (realtime_gl_depth_rbo) glDeleteRenderbuffers(1, &realtime_gl_depth_rbo);
    if (realtime_gl_fbo) glDeleteFramebuffers(1, &realtime_gl_fbo);
    glDeleteTextures(1, &texture);
}
