#include "job_manager.h"

#include <cmath>
#include <iomanip>
#include <fstream>
#include <iterator>
#include <sstream>
#include <thread>
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

bool same_tile_rect(const job_snapshot_t::tile_rect_t &a, const xtcore::render::tile_t *tile)
{
    if (!tile) return false;
    return a.x0 == tile->x0()
        && a.y0 == tile->y0()
        && a.x1 == tile->x1()
        && a.y1 == tile->y1();
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
    , final_fb()
    , image_exr()
    , image_hdr()
    , image_jpg()
    , image_bmp()
    , image_tga()
    , image_raygraph_ply()
    , progressive_fb()
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
            // Start unfinished pixels as fully transparent in progressive previews.
            job->progressive_fb.pixel(x, y) = nimg::ColorRGBAf(0, 0, 0, 0);
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
        [job](common::progress_event_t event, size_t done, size_t total, const xtcore::render::tile_t *tile) {
            {
                std::lock_guard<std::mutex> lock(job->mut);
                if (event == common::PROGRESS_EVENT_TILE_STARTED) {
                    if (tile) {
                        bool exists = false;
                        for (size_t i = 0; i < job->active_tiles.size(); ++i) {
                            if (same_tile_rect(job->active_tiles[i], tile)) {
                                exists = true;
                                break;
                            }
                        }
                        if (!exists) {
                            job_snapshot_t::tile_rect_t rect;
                            rect.x0 = tile->x0();
                            rect.y0 = tile->y0();
                            rect.x1 = tile->x1();
                            rect.y1 = tile->y1();
                            job->active_tiles.push_back(rect);
                        }
                    }
                } else if (event == common::PROGRESS_EVENT_TILE_FINISHED) {
                    copy_tile_to_framebuffer(tile, job->progressive_fb);
                    job->progressive_ready = true;
                    for (size_t i = 0; i < job->active_tiles.size(); ++i) {
                        if (same_tile_rect(job->active_tiles[i], tile)) {
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
        }
    );

    std::lock_guard<std::mutex> lock(job->mut);
    job->elapsed_ms = rr.elapsed_ms;
    if (rr.ok) {
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
    out.width = job->request.width;
    out.height = job->request.height;
    out.active_tiles = job->active_tiles;

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

bool job_manager_t::image(const std::string &id,
                          std::vector<unsigned char> &out,
                          bool allow_partial,
                          const xtcore::tonemapping::settings_t &tm_settings)
{
    std::shared_ptr<job_t> job = get_job(id);
    if (!job) return false;

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

bool job_manager_t::image_export(const std::string &id,
                                 const std::string &format,
                                 std::vector<unsigned char> &out,
                                 std::string &mime_type,
                                 std::string &extension)
{
    std::shared_ptr<job_t> job = get_job(id);
    if (!job) return false;

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

} /* namespace web */
} /* namespace frontend */
} /* namespace xtracer */
