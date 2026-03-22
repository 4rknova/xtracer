#include "job_manager.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <fstream>
#include <iterator>
#include <sstream>
#include <thread>
#include <chrono>
#include <unistd.h>

#include <nimg/img.h>
#include <xtcore/tonemapping/tonemapping.h>

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

bool write_file_bytes(const char *path, const std::vector<unsigned char> &bytes)
{
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out.good()) return false;
    if (!bytes.empty()) {
        out.write((const char *)bytes.data(), (std::streamsize)bytes.size());
    }
    return out.good();
}

typedef int (*save_fn_t)(const char *, nimg::Pixmap &);

bool encode_memory(nimg::Pixmap &pixmap, save_fn_t save_fn, std::vector<unsigned char> &out)
{
    char tmp_path[] = "/tmp/xtracer_web_job_img_XXXXXX";
    int fd = mkstemp(tmp_path);
    if (fd < 0) return false;
    close(fd);

    int save_err = save_fn(tmp_path, pixmap);
    if (save_err != 0) {
        unlink(tmp_path);
        return false;
    }

    bool ok = read_file_bytes(tmp_path, out);
    unlink(tmp_path);
    return ok;
}

bool encode_png_memory(nimg::Pixmap &pixmap,
                       const xtcore::tonemapping::settings_t &tm_settings,
                       std::vector<unsigned char> &out)
{
    nimg::Pixmap ldr = pixmap;
    xtcore::tonemapping::apply(ldr, tm_settings);
    return encode_memory(ldr, nimg::io::save::png, out);
}

bool encode_jpg_memory(nimg::Pixmap &pixmap, std::vector<unsigned char> &out)
{
    nimg::Pixmap ldr = pixmap;
    xtcore::tonemapping::apply(ldr);
    return encode_memory(ldr, nimg::io::save::jpg, out);
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

void copy_rect_from_framebuffer(nimg::Pixmap &src,
                                size_t x0, size_t y0, size_t x1, size_t y1,
                                nimg::Pixmap &dst)
{
    if (x1 <= x0 || y1 <= y0) return;
    const size_t max_w = std::min(src.width(), dst.width());
    const size_t max_h = std::min(src.height(), dst.height());
    if (x0 >= max_w || y0 >= max_h) return;
    const size_t cx1 = std::min(x1, max_w);
    const size_t cy1 = std::min(y1, max_h);
    for (size_t y = y0; y < cy1; ++y) {
        for (size_t x = x0; x < cx1; ++x) {
            dst.pixel(x, y) = src.pixel(x, y);
        }
    }
}

bool same_tile_rect(const job_snapshot_t::tile_rect_t &a, const xtcore::render::tile_t *tile)
{
    if (!tile) return false;
    return a.x0 == tile->x0()
        && a.y0 == tile->y0()
        && a.x1 == tile->x1()
        && a.y1 == tile->y1();
}

unsigned long long parse_job_sequence(const std::string &id)
{
    if (id.size() <= 4 || id.substr(0, 4) != "job_") return 0;
    unsigned long long v = 0;
    for (size_t i = 4; i < id.size(); ++i) {
        const char c = id[i];
        if (c < '0' || c > '9') return 0;
        v = v * 10ULL + (unsigned long long)(c - '0');
    }
    return v;
}

const char *render_mode_label(common::render_request_t::render_mode_t mode)
{
    switch (mode) {
        case common::render_request_t::RENDER_MODE_PROGRESSIVE: return "progressive";
        case common::render_request_t::RENDER_MODE_INTERACTIVE: return "interactive";
        case common::render_request_t::RENDER_MODE_NORMAL:
        default: return "normal";
    }
}

size_t progressive_pass_sample_step(size_t total_samples)
{
    if (total_samples <= 4) return 1;
    if (total_samples <= 16) return 2;
    if (total_samples <= 64) return 4;
    return 8;
}

size_t progressive_pass_count(size_t total_samples)
{
    const size_t samples = (total_samples > 0) ? total_samples : 1;
    const size_t step = progressive_pass_sample_step(samples);
    if (samples <= 1) return 1;
    const size_t rem = samples - 1;
    return 1 + ((rem + step - 1) / step);
}

} // namespace

job_manager_t::job_t::job_t()
    : mut()
    , id()
    , workspace_id()
    , scene()
    , integrator()
    , state(JOB_QUEUED)
    , cancel_requested(false)
    , tiles_done(0)
    , tiles_total(0)
    , error()
    , elapsed_ms(0.0)
    , started_at()
    , has_started(false)
    , image_png()
    , final_fb()
    , image_exr()
    , image_hdr()
    , image_jpg()
    , image_bmp()
    , image_tga()
    , image_raygraph_ply()
    , progressive_fb()
    , finished_tiles()
    , photon_diffuse_points()
    , photon_caustic_points()
    , active_tiles()
    , progressive_ready(false)
    , preview_last_encoded_done(0)
    , preview_last_from_final(false)
    , preview_last_tm_op(xtcore::tonemapping::OP_ACES_FITTED)
    , preview_last_tm_exposure(1.0f)
    , preview_last_tm_white_point(1.0f)
    , preview_last_tm_mantiuk_contrast(0.1f)
    , preview_last_tm_mantiuk_saturation(0.8f)
    , preview_last_tm_mantiuk_detail(1.0f)
    , preview_png_cache()
    , effective_threads(0)
    , request()
    , cleanup_scene_path()
{}

job_manager_t::job_manager_t()
    : jobs_mut()
    , jobs()
    , completed_job_order()
    , max_completed_jobs(8)
    , evicted_jobs()
    , evicted_job_order()
    , max_evicted_jobs(64)
    , next_id(0)
    , render_slots_mut()
    , render_slots_cv()
    , max_concurrent_renders(1)
    , active_renders(0)
    , render_thread_budget(1)
    , active_render_threads(0)
    , queued_job_order()
{}

void job_manager_t::set_max_concurrent_renders(size_t max_concurrent)
{
    if (max_concurrent == 0) max_concurrent = 1;
    std::lock_guard<std::mutex> lock(render_slots_mut);
    max_concurrent_renders = max_concurrent;
    render_slots_cv.notify_all();
}

void job_manager_t::set_render_thread_budget(size_t max_threads)
{
    if (max_threads == 0) max_threads = 1;
    std::lock_guard<std::mutex> lock(render_slots_mut);
    render_thread_budget = max_threads;
    render_slots_cv.notify_all();
}

size_t job_manager_t::get_max_concurrent_renders() const
{
    std::lock_guard<std::mutex> lock(render_slots_mut);
    return max_concurrent_renders;
}

size_t job_manager_t::get_active_render_count() const
{
    std::lock_guard<std::mutex> lock(render_slots_mut);
    return active_renders;
}

std::string job_manager_t::create(const common::render_request_t &request,
                                  const std::string &scene_name,
                                  const std::string &workspace_id,
                                  const std::string &cleanup_scene_path)
{
    std::shared_ptr<job_t> job(new job_t());
    unsigned long long id = ++next_id;

    std::ostringstream ss;
    ss << "job_" << id;
    job->id = ss.str();
    job->workspace_id = workspace_id;
    job->scene = scene_name;
    job->integrator = request.integrator;
    job->request = request;
    job->cleanup_scene_path = cleanup_scene_path;
    job->progressive_fb.init(request.width, request.height);
    for (size_t y = 0; y < request.height; ++y) {
        for (size_t x = 0; x < request.width; ++x) {
            // Start unfinished pixels as fully transparent in progressive previews.
            job->progressive_fb.pixel(x, y) = nimg::ColorRGBAf(0, 0, 0, 0);
        }
    }

    {
        std::lock_guard<std::mutex> lock(jobs_mut);
        jobs[job->id] = job;
    }
    {
        std::lock_guard<std::mutex> lock(render_slots_mut);
        queued_job_order.push_back(job->id);
        render_slots_cv.notify_all();
    }

    std::thread t(&job_manager_t::run, this, job);
    t.detach();

    std::ostringstream log;
    log << "job accepted id=" << job->id
        << " workspace=" << workspace_id
        << " scene=" << scene_name
        << " integrator=" << request.integrator
        << " mode=" << render_mode_label(request.render_mode);
    backend_log_t::handle().add("info", log.str());

    return job->id;
}

void job_manager_t::run(const std::shared_ptr<job_t> &job)
{
    if (!job) return;
    const size_t requested_threads = job->request.threads;
    size_t granted_threads = 1;

    if (job->cancel_requested.load()) {
        job->state = JOB_ABORTED;
        job->error = "render aborted";
        on_job_finished(job->id);
        return;
    }

    {
        std::unique_lock<std::mutex> lock(render_slots_mut);
        render_slots_cv.wait(lock, [this, job, requested_threads]() {
            if (job->cancel_requested.load()) return true;
            if (active_renders >= max_concurrent_renders) return false;
            if (queued_job_order.empty()) return false;
            if (requested_threads > 0) {
                if (active_render_threads + requested_threads > render_thread_budget) return false;
            } else {
                // Auto mode: allow start as soon as at least one render thread is free.
                if (active_render_threads >= render_thread_budget) return false;
            }
            return queued_job_order.front() == job->id;
        });
        if (job->cancel_requested.load()) {
            for (auto it = queued_job_order.begin(); it != queued_job_order.end(); ++it) {
                if (*it == job->id) {
                    queued_job_order.erase(it);
                    break;
                }
            }
            lock.unlock();
            std::lock_guard<std::mutex> job_lock(job->mut);
            const job_state_t st = job->state.load();
            if (st == JOB_QUEUED || st == JOB_RUNNING) {
                job->error = "render aborted";
                job->state = JOB_ABORTED;
                backend_log_t::handle().add("info", "job aborted id=" + job->id + " state=queued");
                on_job_finished(job->id);
            }
            return;
        }
        if (!queued_job_order.empty() && queued_job_order.front() == job->id) {
            queued_job_order.pop_front();
        }
        if (requested_threads > 0) {
            granted_threads = requested_threads;
        } else {
            const size_t free_threads = (render_thread_budget > active_render_threads)
                ? (render_thread_budget - active_render_threads)
                : 0;
            granted_threads = (free_threads > 0) ? free_threads : 1;
        }
        ++active_renders;
        active_render_threads += granted_threads;
        render_slots_cv.notify_all();
    }

    job->state = JOB_RUNNING;
    {
        std::lock_guard<std::mutex> lock(job->mut);
        job->started_at = std::chrono::steady_clock::now();
        job->has_started = true;
        job->effective_threads = granted_threads;
    }
    backend_log_t::handle().add("info", "job started id=" + job->id);

    struct render_slot_guard_t {
        std::mutex &mut;
        std::condition_variable &cv;
        size_t &active;
        size_t &active_threads;
        const size_t release_threads;
        render_slot_guard_t(std::mutex &m, std::condition_variable &c, size_t &a, size_t &at, size_t rt)
            : mut(m), cv(c), active(a), active_threads(at), release_threads(rt) {}
        ~render_slot_guard_t() {
            {
                std::lock_guard<std::mutex> lock(mut);
                if (active > 0) --active;
                if (active_threads >= release_threads) active_threads -= release_threads;
                else active_threads = 0;
            }
            cv.notify_one();
        }
    } render_slot_guard(render_slots_mut, render_slots_cv, active_renders, active_render_threads, granted_threads);

    common::render_request_t request = job->request;
    request.threads = granted_threads;
    common::render_result_t rr = common::render_scene_to_png(request,
        [job](common::progress_event_t event, size_t done, size_t total, const xtcore::render::tile_t *tile, const common::progress_tile_update_t *upd) {
            {
                std::lock_guard<std::mutex> lock(job->mut);
                if (event == common::PROGRESS_EVENT_TILE_STARTED) {
                    const bool has_upd_rect = upd && upd->has_rect;
                    if (tile || has_upd_rect) {
                        const size_t rx0 = has_upd_rect ? upd->x0 : tile->x0();
                        const size_t ry0 = has_upd_rect ? upd->y0 : tile->y0();
                        const size_t rx1 = has_upd_rect ? upd->x1 : tile->x1();
                        const size_t ry1 = has_upd_rect ? upd->y1 : tile->y1();
                        bool exists = false;
                        for (size_t i = 0; i < job->active_tiles.size(); ++i) {
                            const job_snapshot_t::tile_rect_t &r = job->active_tiles[i];
                            if (r.x0 == rx0 && r.y0 == ry0 && r.x1 == rx1 && r.y1 == ry1) {
                                exists = true;
                                break;
                            }
                        }
                        if (!exists) {
                            job_snapshot_t::tile_rect_t rect;
                            rect.x0 = rx0;
                            rect.y0 = ry0;
                            rect.x1 = rx1;
                            rect.y1 = ry1;
                            job->active_tiles.push_back(rect);
                        }
                    }
                } else if (event == common::PROGRESS_EVENT_TILE_FINISHED) {
                    const bool has_upd_rect = upd && upd->has_rect;
                    if (has_upd_rect && upd->source_fb) {
                        copy_rect_from_framebuffer(*upd->source_fb, upd->x0, upd->y0, upd->x1, upd->y1, job->progressive_fb);
                        job->progressive_ready = true;
                    } else {
                        copy_tile_to_framebuffer(tile, job->progressive_fb);
                        if (tile) job->progressive_ready = true;
                    }
                    if (tile || has_upd_rect) {
                        job_t::finished_tile_t finished;
                        finished.rect.x0 = has_upd_rect ? upd->x0 : tile->x0();
                        finished.rect.y0 = has_upd_rect ? upd->y0 : tile->y0();
                        finished.rect.x1 = has_upd_rect ? upd->x1 : tile->x1();
                        finished.rect.y1 = has_upd_rect ? upd->y1 : tile->y1();
                        finished.done_index = done;
                        job->finished_tiles.push_back(finished);
                    }
                    for (size_t i = 0; i < job->active_tiles.size(); ++i) {
                        const bool match_tile = tile && same_tile_rect(job->active_tiles[i], tile);
                        const bool match_upd = has_upd_rect
                            && job->active_tiles[i].x0 == upd->x0
                            && job->active_tiles[i].y0 == upd->y0
                            && job->active_tiles[i].x1 == upd->x1
                            && job->active_tiles[i].y1 == upd->y1;
                        if (match_tile || match_upd) {
                            job->active_tiles.erase(job->active_tiles.begin() + i);
                            break;
                        }
                    }
                }
            }
            job->tiles_total = total;
            if (event == common::PROGRESS_EVENT_TILE_FINISHED) {
                job->tiles_done = done;
            }
        },
        &job->cancel_requested
    );

    {
        std::lock_guard<std::mutex> lock(job->mut);
        job->elapsed_ms = rr.elapsed_ms;
        if (rr.aborted || job->cancel_requested.load()) {
            job->error = "render aborted";
            job->active_tiles.clear();
            job->state = JOB_ABORTED;
            backend_log_t::handle().add("info", "job aborted id=" + job->id);
        } else if (rr.ok) {
            job->image_png.swap(rr.image_png);
            job->image_raygraph_ply.swap(rr.raygraph_ply);
            job->final_fb = rr.framebuffer;
            job->image_exr.clear();
            job->image_hdr.clear();
            job->image_jpg.clear();
            job->image_bmp.clear();
            job->image_tga.clear();
            job->photon_diffuse_points = rr.photon_diffuse_points;
            job->photon_caustic_points = rr.photon_caustic_points;
            job->tiles_done = rr.tiles_done;
            job->tiles_total = rr.tiles_total;
            job->preview_png_cache.clear();
            job->preview_last_encoded_done = 0;
            job->preview_last_from_final = false;
            job->preview_last_tm_op = xtcore::tonemapping::OP_ACES_FITTED;
            job->preview_last_tm_exposure = 1.0f;
            job->preview_last_tm_white_point = 1.0f;
            job->preview_last_tm_mantiuk_contrast = 0.1f;
            job->preview_last_tm_mantiuk_saturation = 0.8f;
            job->preview_last_tm_mantiuk_detail = 1.0f;
            job->active_tiles.clear();
            job->state = JOB_DONE;
            std::ostringstream log;
            log << "job completed id=" << job->id
                << " elapsed_ms=" << std::fixed << std::setprecision(0) << rr.elapsed_ms;
            backend_log_t::handle().add("info", log.str());
        } else {
            job->error = rr.error;
            job->active_tiles.clear();
            job->state = JOB_ERROR;
            backend_log_t::handle().add("error", "job failed id=" + job->id + " reason=" + rr.error);
        }
    }

    if (!job->cleanup_scene_path.empty()) {
        unlink(job->cleanup_scene_path.c_str());
    }
    on_job_finished(job->id);
}

std::shared_ptr<job_manager_t::job_t> job_manager_t::get_job(const std::string &id)
{
    std::lock_guard<std::mutex> lock(jobs_mut);
    auto it = jobs.find(id);
    if (it == jobs.end()) return std::shared_ptr<job_t>();
    return it->second;
}

void job_manager_t::prune_completed_jobs_locked()
{
    while (completed_job_order.size() > max_completed_jobs) {
        const std::string evict_id = completed_job_order.front();
        completed_job_order.pop_front();

        auto it = jobs.find(evict_id);
        if (it == jobs.end()) continue;

        const job_state_t st = it->second->state.load();
        if (st == JOB_RUNNING || st == JOB_QUEUED) continue;

        cache_evicted_job_locked(it->second);
        jobs.erase(it);
        backend_log_t::handle().add("debug", "job evicted id=" + evict_id);
    }
}

void job_manager_t::cache_evicted_job_locked(const std::shared_ptr<job_t> &job)
{
    if (!job) return;

    evicted_job_t rec;
    rec.id = job->id;
    rec.workspace_id = job->workspace_id;
    rec.scene = job->scene;
    rec.integrator = job->integrator;
    rec.state = job->state.load();
    rec.error = job->error;
    rec.elapsed_ms = job->elapsed_ms;
    rec.width = job->request.width;
    rec.height = job->request.height;
    rec.threads = (job->effective_threads > 0) ? job->effective_threads : job->request.threads;

    {
        std::lock_guard<std::mutex> lock(job->mut);
        if (!job->image_png.empty()) {
            std::string png_path = "/tmp/xtracer_web_job_cache_" + job->id + ".png";
            if (write_file_bytes(png_path.c_str(), job->image_png)) {
                rec.png_path = png_path;
            }
        }
    }

    evicted_jobs[rec.id] = rec;
    evicted_job_order.push_back(rec.id);
    prune_evicted_jobs_locked();
}

void job_manager_t::prune_evicted_jobs_locked()
{
    while (evicted_job_order.size() > max_evicted_jobs) {
        const std::string id = evicted_job_order.front();
        evicted_job_order.pop_front();
        auto it = evicted_jobs.find(id);
        if (it == evicted_jobs.end()) continue;
        if (!it->second.png_path.empty()) {
            unlink(it->second.png_path.c_str());
        }
        evicted_jobs.erase(it);
    }
}

void job_manager_t::on_job_finished(const std::string &id)
{
    std::lock_guard<std::mutex> lock(jobs_mut);
    auto it = jobs.find(id);
    if (it == jobs.end()) return;
    const job_state_t st = it->second->state.load();
    if (st != JOB_DONE && st != JOB_ABORTED && st != JOB_ERROR) return;
    completed_job_order.push_back(id);
    prune_completed_jobs_locked();
}

bool job_manager_t::snapshot(const std::string &id, job_snapshot_t &out)
{
    std::shared_ptr<job_t> job = get_job(id);
    if (!job) {
        std::lock_guard<std::mutex> lock(jobs_mut);
        auto eit = evicted_jobs.find(id);
        if (eit == evicted_jobs.end()) return false;
        const evicted_job_t &e = eit->second;
        out.id = e.id;
        out.workspace_id = e.workspace_id;
        out.scene = e.scene;
        out.integrator = e.integrator;
        out.render_mode = "normal";
        out.state = e.state;
        out.error = e.error;
        out.elapsed_ms = e.elapsed_ms;
        out.has_image = !e.png_path.empty();
        out.width = e.width;
        out.height = e.height;
        out.threads = e.threads;
        out.tiles_done = 0;
        out.tiles_total = 0;
        out.pass_current = 0;
        out.pass_total = 0;
        out.queue_index = -1;
        out.active_tiles.clear();
        out.progress = (e.state == JOB_DONE) ? 1.0f : 0.0f;
        return true;
    }

    std::lock_guard<std::mutex> lock(job->mut);

    out.id = job->id;
    out.workspace_id = job->workspace_id;
    out.scene = job->scene;
    out.integrator = job->integrator;
    out.render_mode = render_mode_label(job->request.render_mode);
    out.state = job->state.load();
    out.error = job->error;
    out.elapsed_ms = job->elapsed_ms;
    if ((out.state == JOB_RUNNING || out.state == JOB_QUEUED) && job->has_started) {
        const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        out.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - job->started_at).count();
    }
    out.has_image = !job->image_png.empty();
    out.width = job->request.width;
    out.height = job->request.height;
    out.threads = (job->effective_threads > 0) ? job->effective_threads : job->request.threads;
    out.tiles_done = job->tiles_done.load();
    out.tiles_total = job->tiles_total.load();
    out.pass_current = 0;
    out.pass_total = 0;
    out.queue_index = -1;
    out.active_tiles = job->active_tiles;

    if (job->request.render_mode == common::render_request_t::RENDER_MODE_PROGRESSIVE) {
        const size_t ptotal = progressive_pass_count(job->request.samples);
        out.pass_total = ptotal;
        size_t tiles_per_pass = (ptotal > 0) ? (out.tiles_total / ptotal) : 0;
        if (tiles_per_pass == 0) {
            const size_t tile_size = (job->request.tile_size > 0) ? job->request.tile_size : 1;
            const size_t nx = (job->request.width + tile_size - 1) / tile_size;
            const size_t ny = (job->request.height + tile_size - 1) / tile_size;
            tiles_per_pass = nx * ny;
        }
        if (out.state == JOB_DONE) {
            out.pass_current = out.pass_total;
        } else if (out.pass_total > 0) {
            const size_t completed_passes = (tiles_per_pass > 0) ? (out.tiles_done / tiles_per_pass) : 0;
            const bool in_pass = (tiles_per_pass > 0) ? ((out.tiles_done % tiles_per_pass) != 0) : false;
            size_t curr = completed_passes + (in_pass ? 1 : 0);
            if ((out.state == JOB_QUEUED || out.state == JOB_RUNNING) && curr == 0) curr = 1;
            if (curr > out.pass_total) curr = out.pass_total;
            out.pass_current = curr;
        }
    }

    size_t total = out.tiles_total;
    size_t done = out.tiles_done;
    if (total == 0) {
        out.progress = (out.state == JOB_DONE) ? 1.0f : 0.0f;
    } else {
        out.progress = (float)done / (float)total;
        if (out.progress > 1.0f) out.progress = 1.0f;
    }

    return true;
}

bool job_manager_t::image(const std::string &id,
                          std::vector<unsigned char> &out,
                          bool allow_partial,
                          const xtcore::tonemapping::settings_t &tm_settings)
{
    std::shared_ptr<job_t> job = get_job(id);
    if (!job) {
        if (allow_partial) return false;
        std::string png_path;
        {
            std::lock_guard<std::mutex> lock(jobs_mut);
            auto eit = evicted_jobs.find(id);
            if (eit == evicted_jobs.end() || eit->second.png_path.empty()) return false;
            png_path = eit->second.png_path;
        }
        return read_file_bytes(png_path.c_str(), out);
    }

    std::lock_guard<std::mutex> lock(job->mut);

    const bool use_final = (job->state == JOB_DONE
                            && job->final_fb.width() > 0
                            && job->final_fb.height() > 0);
    if (!use_final && (!allow_partial || !job->progressive_ready)) return false;

    const nimg::Pixmap &src = use_final ? job->final_fb : job->progressive_fb;
    const size_t done = use_final ? job->tiles_total.load() : job->tiles_done.load();
    const bool cache_invalid = job->preview_png_cache.empty()
                            || job->preview_last_encoded_done != done
                            || job->preview_last_from_final != use_final
                            || job->preview_last_tm_op != tm_settings.op
                            || std::fabs(job->preview_last_tm_exposure - tm_settings.exposure) > 1e-6f
                            || std::fabs(job->preview_last_tm_white_point - tm_settings.white_point) > 1e-6f
                            || std::fabs(job->preview_last_tm_mantiuk_contrast - tm_settings.mantiuk_contrast) > 1e-6f
                            || std::fabs(job->preview_last_tm_mantiuk_saturation - tm_settings.mantiuk_saturation) > 1e-6f
                            || std::fabs(job->preview_last_tm_mantiuk_detail - tm_settings.mantiuk_detail) > 1e-6f;

    if (cache_invalid) {
        nimg::Pixmap work = src;
        if (!encode_png_memory(work, tm_settings, job->preview_png_cache)) return false;
        job->preview_last_encoded_done = done;
        job->preview_last_from_final = use_final;
        job->preview_last_tm_op = tm_settings.op;
        job->preview_last_tm_exposure = tm_settings.exposure;
        job->preview_last_tm_white_point = tm_settings.white_point;
        job->preview_last_tm_mantiuk_contrast = tm_settings.mantiuk_contrast;
        job->preview_last_tm_mantiuk_saturation = tm_settings.mantiuk_saturation;
        job->preview_last_tm_mantiuk_detail = tm_settings.mantiuk_detail;
    }

    out = job->preview_png_cache;
    return true;
}

bool job_manager_t::image_delta(const std::string &id,
                                size_t since_done,
                                size_t max_tiles,
                                const xtcore::tonemapping::settings_t &tm_settings,
                                job_image_delta_t &out)
{
    std::shared_ptr<job_t> job = get_job(id);
    if (!job) return false;

    struct pending_tile_t {
        size_t x0;
        size_t y0;
        size_t x1;
        size_t y1;
        size_t done_index;
        nimg::Pixmap tile_fb;
    };

    std::vector<pending_tile_t> pending;
    pending.reserve(max_tiles);

    {
        std::lock_guard<std::mutex> lock(job->mut);
        out.state = job->state.load();
        out.width = job->request.width;
        out.height = job->request.height;
        out.tiles_done = job->tiles_done.load();
        out.tiles_total = job->tiles_total.load();
        out.tiles.clear();

        const bool use_final = (out.state == JOB_DONE
                                && job->final_fb.width() > 0
                                && job->final_fb.height() > 0);
        if (!use_final && !job->progressive_ready) return true;

        nimg::Pixmap &src = use_final ? job->final_fb : job->progressive_fb;
        for (size_t i = 0; i < job->finished_tiles.size(); ++i) {
            const job_t::finished_tile_t &t = job->finished_tiles[i];
            if (t.done_index <= since_done) continue;
            if (max_tiles > 0 && pending.size() >= max_tiles) break;

            const size_t tw = (t.rect.x1 > t.rect.x0) ? (t.rect.x1 - t.rect.x0) : 0;
            const size_t th = (t.rect.y1 > t.rect.y0) ? (t.rect.y1 - t.rect.y0) : 0;
            if (tw == 0 || th == 0) continue;

            pending_tile_t entry;
            entry.x0 = t.rect.x0;
            entry.y0 = t.rect.y0;
            entry.x1 = t.rect.x1;
            entry.y1 = t.rect.y1;
            entry.done_index = t.done_index;
            entry.tile_fb.init(tw, th);
            for (size_t y = 0; y < th; ++y) {
                for (size_t x = 0; x < tw; ++x) {
                    entry.tile_fb.pixel(x, y) = src.pixel(t.rect.x0 + x, t.rect.y0 + y);
                }
            }
            pending.push_back(entry);
        }
    }

    out.tiles.reserve(pending.size());
    for (size_t i = 0; i < pending.size(); ++i) {
        std::vector<unsigned char> encoded;
        if (!encode_png_memory(pending[i].tile_fb, tm_settings, encoded)) continue;

        job_image_delta_t::tile_t tile;
        tile.x0 = pending[i].x0;
        tile.y0 = pending[i].y0;
        tile.x1 = pending[i].x1;
        tile.y1 = pending[i].y1;
        tile.done_index = pending[i].done_index;
        tile.png.swap(encoded);
        out.tiles.push_back(tile);
    }

    return true;
}

bool job_manager_t::image_export(const std::string &id,
                                 const std::string &format,
                                 std::vector<unsigned char> &out,
                                 std::string &mime_type,
                                 std::string &extension)
{
    std::shared_ptr<job_t> job = get_job(id);
    if (!job) {
        if (format != "png") return false;
        std::string png_path;
        {
            std::lock_guard<std::mutex> lock(jobs_mut);
            auto eit = evicted_jobs.find(id);
            if (eit == evicted_jobs.end() || eit->second.png_path.empty()) return false;
            png_path = eit->second.png_path;
        }
        if (!read_file_bytes(png_path.c_str(), out)) return false;
        mime_type = "image/png";
        extension = "png";
        return true;
    }

    std::lock_guard<std::mutex> lock(job->mut);
    if (job->state != JOB_DONE) return false;

    if (format == "png") {
        if (job->image_png.empty()) return false;
        out = job->image_png;
        mime_type = "image/png";
        extension = "png";
        return true;
    }

    if (job->final_fb.width() == 0 || job->final_fb.height() == 0) return false;

    if (format == "exr") {
        if (job->image_exr.empty()) {
            if (!encode_memory(job->final_fb, nimg::io::save::exr, job->image_exr)) return false;
        }
        out = job->image_exr;
        mime_type = "image/x-exr";
        extension = "exr";
        return true;
    }

    if (format == "hdr") {
        if (job->image_hdr.empty()) {
            if (!encode_memory(job->final_fb, nimg::io::save::hdr, job->image_hdr)) return false;
        }
        out = job->image_hdr;
        mime_type = "image/vnd.radiance";
        extension = "hdr";
        return true;
    }

    if (format == "jpg") {
        if (job->image_jpg.empty()) {
            if (!encode_jpg_memory(job->final_fb, job->image_jpg)) return false;
        }
        out = job->image_jpg;
        mime_type = "image/jpeg";
        extension = "jpg";
        return true;
    }

    if (format == "bmp") {
        if (job->image_bmp.empty()) {
            if (!encode_memory(job->final_fb, nimg::io::save::bmp, job->image_bmp)) return false;
        }
        out = job->image_bmp;
        mime_type = "image/bmp";
        extension = "bmp";
        return true;
    }

    if (format == "tga") {
        if (job->image_tga.empty()) {
            if (!encode_memory(job->final_fb, nimg::io::save::tga, job->image_tga)) return false;
        }
        out = job->image_tga;
        mime_type = "image/x-tga";
        extension = "tga";
        return true;
    }

    if (format == "ply") {
        if (job->image_raygraph_ply.empty()) return false;
        out = job->image_raygraph_ply;
        mime_type = "application/octet-stream";
        extension = "ply";
        return true;
    }

    return false;
}

bool job_manager_t::photons(const std::string &id,
                            std::vector<common::render_result_t::point3_t> &diffuse_out,
                            std::vector<common::render_result_t::point3_t> &caustic_out,
                            size_t limit_per_set)
{
    std::shared_ptr<job_t> job = get_job(id);
    if (!job) return false;

    std::lock_guard<std::mutex> lock(job->mut);
    if (job->state != JOB_DONE) return false;
    if (job->integrator != "photon_mapping") return false;

    const size_t cap = (limit_per_set == 0) ? (size_t)100000 : limit_per_set;
    auto append_capped = [cap](const std::vector<common::render_result_t::point3_t> &src,
                               std::vector<common::render_result_t::point3_t> &dst) {
        dst.clear();
        if (src.empty()) return;
        if (src.size() <= cap) {
            dst = src;
            return;
        }
        dst.reserve(cap);
        const double step = (double)src.size() / (double)cap;
        for (size_t i = 0; i < cap; ++i) {
            size_t idx = (size_t)(i * step);
            if (idx >= src.size()) idx = src.size() - 1;
            dst.push_back(src[idx]);
        }
    };

    append_capped(job->photon_diffuse_points, diffuse_out);
    append_capped(job->photon_caustic_points, caustic_out);
    return true;
}

bool job_manager_t::abort(const std::string &id)
{
    std::shared_ptr<job_t> job = get_job(id);
    if (!job) return false;

    job->cancel_requested.store(true);
    {
        std::lock_guard<std::mutex> lock(render_slots_mut);
        for (auto it = queued_job_order.begin(); it != queued_job_order.end(); ++it) {
            if (*it == id) {
                queued_job_order.erase(it);
                break;
            }
        }
        render_slots_cv.notify_all();
    }
    const job_state_t st = job->state.load();
    if (st == JOB_QUEUED) {
        std::lock_guard<std::mutex> lock(job->mut);
        if (job->state.load() == JOB_QUEUED) {
            job->error = "render aborted";
            job->state = JOB_ABORTED;
            if (!job->cleanup_scene_path.empty()) {
                unlink(job->cleanup_scene_path.c_str());
                job->cleanup_scene_path.clear();
            }
            backend_log_t::handle().add("info", "job aborted id=" + job->id + " state=queued");
            on_job_finished(job->id);
        }
    }
    return true;
}

bool job_manager_t::move_queue_up(const std::string &id)
{
    std::shared_ptr<job_t> job = get_job(id);
    if (!job) return false;
    if (job->state.load() != JOB_QUEUED) return false;

    std::lock_guard<std::mutex> lock(render_slots_mut);
    for (size_t i = 0; i < queued_job_order.size(); ++i) {
        if (queued_job_order[i] != id) continue;
        if (i == 0) return true;
        std::swap(queued_job_order[i - 1], queued_job_order[i]);
        render_slots_cv.notify_all();
        return true;
    }
    return false;
}

bool job_manager_t::move_queue_down(const std::string &id)
{
    std::shared_ptr<job_t> job = get_job(id);
    if (!job) return false;
    if (job->state.load() != JOB_QUEUED) return false;

    std::lock_guard<std::mutex> lock(render_slots_mut);
    for (size_t i = 0; i < queued_job_order.size(); ++i) {
        if (queued_job_order[i] != id) continue;
        if (i + 1 >= queued_job_order.size()) return true;
        std::swap(queued_job_order[i], queued_job_order[i + 1]);
        render_slots_cv.notify_all();
        return true;
    }
    return false;
}

bool job_manager_t::list_active(std::vector<job_snapshot_t> &out)
{
    std::vector<std::string> running_ids;
    std::vector<std::string> queued_ids;
    std::map<std::string, int> queue_index_by_id;

    {
        std::lock_guard<std::mutex> lock(render_slots_mut);
        for (size_t i = 0; i < queued_job_order.size(); ++i) {
            queue_index_by_id[queued_job_order[i]] = static_cast<int>(i);
        }
    }

    {
        std::lock_guard<std::mutex> lock(jobs_mut);
        for (auto it = jobs.begin(); it != jobs.end(); ++it) {
            const std::shared_ptr<job_t> &job = it->second;
            if (!job) continue;
            const job_state_t st = job->state.load();
            if (st == JOB_RUNNING) {
                running_ids.push_back(job->id);
            } else if (st == JOB_QUEUED) {
                queued_ids.push_back(job->id);
            }
        }
    }

    std::sort(running_ids.begin(), running_ids.end(), [](const std::string &a, const std::string &b) {
        const unsigned long long sa = parse_job_sequence(a);
        const unsigned long long sb = parse_job_sequence(b);
        if (sa != sb) return sa > sb;
        return a > b;
    });
    std::sort(queued_ids.begin(), queued_ids.end(), [&queue_index_by_id](const std::string &a, const std::string &b) {
        const auto ia = queue_index_by_id.find(a);
        const auto ib = queue_index_by_id.find(b);
        const int qa = (ia == queue_index_by_id.end()) ? 0x3fffffff : ia->second;
        const int qb = (ib == queue_index_by_id.end()) ? 0x3fffffff : ib->second;
        if (qa != qb) return qa < qb;
        const unsigned long long sa = parse_job_sequence(a);
        const unsigned long long sb = parse_job_sequence(b);
        if (sa != sb) return sa > sb;
        return a > b;
    });

    out.clear();
    for (size_t i = 0; i < running_ids.size(); ++i) {
        job_snapshot_t snap;
        if (snapshot(running_ids[i], snap)) {
            snap.queue_index = -1;
            out.push_back(snap);
        }
    }
    for (size_t i = 0; i < queued_ids.size(); ++i) {
        job_snapshot_t snap;
        if (snapshot(queued_ids[i], snap)) {
            const auto qit = queue_index_by_id.find(snap.id);
            snap.queue_index = (qit == queue_index_by_id.end()) ? static_cast<int>(i) : qit->second;
            out.push_back(snap);
        }
    }
    return !out.empty();
}

} /* namespace web */
} /* namespace frontend */
} /* namespace xtracer */
