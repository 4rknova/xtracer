#include "job_manager.h"

#include <iomanip>
#include <fstream>
#include <iterator>
#include <sstream>
#include <thread>
#include <unistd.h>

#include <nimg/img.h>

#include "backend_log.h"

namespace xtracer {
namespace frontend {
namespace web {

namespace {

bool read_file_bytes(const char *path, std::vector<unsigned char> &out)
{
    std::ifstream in(path, std::ios::binary);
    if (!in.good()) return false;
    out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return true;
}

bool encode_png_memory(nimg::Pixmap &pixmap, std::vector<unsigned char> &out)
{
    char tmp_path[] = "/tmp/xtracer_web_job_png_XXXXXX";
    int fd = mkstemp(tmp_path);
    if (fd < 0) return false;
    close(fd);

    int png_err = nimg::io::save::png(tmp_path, pixmap);
    if (png_err != 0) {
        unlink(tmp_path);
        return false;
    }

    bool ok = read_file_bytes(tmp_path, out);
    unlink(tmp_path);
    return ok;
}

void copy_tile_to_framebuffer(const xtcore::render::tile_t *tile, nimg::Pixmap &fb)
{
    if (!tile) return;
    nimg::ColorRGBAf col;
    for (size_t y = tile->y0(); y < tile->y1(); ++y) {
        for (size_t x = tile->x0(); x < tile->x1(); ++x) {
            tile->read(x, y, col);
            fb.pixel(x, y) = col;
        }
    }
}

} // namespace

job_manager_t::job_t::job_t()
    : mut()
    , id()
    , scene()
    , integrator()
    , state(JOB_QUEUED)
    , tiles_done(0)
    , tiles_total(0)
    , error()
    , elapsed_ms(0.0)
    , image_png()
    , progressive_fb()
    , progressive_ready(false)
    , last_encoded_done(0)
    , progressive_png_cache()
    , request()
{}

job_manager_t::job_manager_t()
    : jobs_mut()
    , jobs()
    , next_id(0)
    , render_mut()
{}

std::string job_manager_t::create(const common::render_request_t &request, const std::string &scene_name)
{
    std::shared_ptr<job_t> job(new job_t());
    unsigned long long id = ++next_id;

    std::ostringstream ss;
    ss << "job_" << id;
    job->id = ss.str();
    job->scene = scene_name;
    job->integrator = request.integrator;
    job->request = request;
    job->progressive_fb.init(request.width, request.height);
    for (size_t y = 0; y < request.height; ++y) {
        for (size_t x = 0; x < request.width; ++x) {
            job->progressive_fb.pixel(x, y) = nimg::ColorRGBAf(0, 0, 0, 1);
        }
    }

    {
        std::lock_guard<std::mutex> lock(jobs_mut);
        jobs[job->id] = job;
    }

    std::thread t(&job_manager_t::run, this, job);
    t.detach();

    std::ostringstream log;
    log << "job accepted id=" << job->id
        << " scene=" << scene_name
        << " integrator=" << request.integrator;
    backend_log_t::handle().add("info", log.str());

    return job->id;
}

void job_manager_t::run(const std::shared_ptr<job_t> &job)
{
    if (!job) return;

    job->state = JOB_RUNNING;
    backend_log_t::handle().add("info", "job started id=" + job->id);

    std::lock_guard<std::mutex> render_lock(render_mut);

    common::render_result_t rr = common::render_scene_to_png(job->request,
        [job](size_t done, size_t total, const xtcore::render::tile_t *tile) {
            {
                std::lock_guard<std::mutex> lock(job->mut);
                copy_tile_to_framebuffer(tile, job->progressive_fb);
                job->progressive_ready = true;
            }
            job->tiles_done = done;
            job->tiles_total = total;
        }
    );

    std::lock_guard<std::mutex> lock(job->mut);
    job->elapsed_ms = rr.elapsed_ms;
    if (rr.ok) {
        job->image_png.swap(rr.image_png);
        job->tiles_done = rr.tiles_done;
        job->tiles_total = rr.tiles_total;
        job->progressive_png_cache = job->image_png;
        job->last_encoded_done = job->tiles_done.load();
        job->state = JOB_DONE;
        std::ostringstream log;
        log << "job completed id=" << job->id
            << " elapsed_ms=" << std::fixed << std::setprecision(0) << rr.elapsed_ms;
        backend_log_t::handle().add("info", log.str());
    } else {
        job->error = rr.error;
        job->state = JOB_ERROR;
        backend_log_t::handle().add("error", "job failed id=" + job->id + " reason=" + rr.error);
    }
}

std::shared_ptr<job_manager_t::job_t> job_manager_t::get_job(const std::string &id)
{
    std::lock_guard<std::mutex> lock(jobs_mut);
    auto it = jobs.find(id);
    if (it == jobs.end()) return std::shared_ptr<job_t>();
    return it->second;
}

bool job_manager_t::snapshot(const std::string &id, job_snapshot_t &out)
{
    std::shared_ptr<job_t> job = get_job(id);
    if (!job) return false;

    std::lock_guard<std::mutex> lock(job->mut);

    out.id = job->id;
    out.scene = job->scene;
    out.integrator = job->integrator;
    out.state = job->state.load();
    out.error = job->error;
    out.elapsed_ms = job->elapsed_ms;
    out.has_image = !job->image_png.empty();

    size_t total = job->tiles_total.load();
    size_t done = job->tiles_done.load();
    if (total == 0) {
        out.progress = (out.state == JOB_DONE) ? 1.0f : 0.0f;
    } else {
        out.progress = (float)done / (float)total;
        if (out.progress > 1.0f) out.progress = 1.0f;
    }

    return true;
}

bool job_manager_t::image(const std::string &id, std::vector<unsigned char> &out, bool allow_partial)
{
    std::shared_ptr<job_t> job = get_job(id);
    if (!job) return false;

    std::lock_guard<std::mutex> lock(job->mut);
    if (job->state == JOB_DONE && !job->image_png.empty()) {
        out = job->image_png;
        return true;
    }

    if (!allow_partial || !job->progressive_ready) return false;

    size_t done = job->tiles_done.load();
    if (job->progressive_png_cache.empty() || job->last_encoded_done != done) {
        if (!encode_png_memory(job->progressive_fb, job->progressive_png_cache)) return false;
        job->last_encoded_done = done;
    }

    out = job->progressive_png_cache;
    return true;
}

} /* namespace web */
} /* namespace frontend */
} /* namespace xtracer */
