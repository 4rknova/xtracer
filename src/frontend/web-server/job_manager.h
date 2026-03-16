#ifndef XTRACER_FRONTEND_WEB_JOB_MANAGER_H_INCLUDED
#define XTRACER_FRONTEND_WEB_JOB_MANAGER_H_INCLUDED

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <deque>
#include <atomic>
#include <condition_variable>

#include <frontend/common/render_service.h>
#include <nimg/pixmap.h>
#include <xtcore/tonemapping/tonemapping.h>

namespace xtracer {
namespace frontend {
namespace web {

enum job_state_t
{
    JOB_QUEUED = 0,
    JOB_RUNNING,
    JOB_DONE,
    JOB_ERROR
};

struct job_snapshot_t
{
    struct tile_rect_t {
        size_t x0;
        size_t y0;
        size_t x1;
        size_t y1;
    };

    std::string id;
    std::string workspace_id;
    std::string scene;
    std::string integrator;
    job_state_t state;
    float progress;
    std::string error;
    double elapsed_ms;
    bool has_image;
    size_t width;
    size_t height;
    std::vector<tile_rect_t> active_tiles;
};

struct job_image_delta_t
{
    struct tile_t {
        size_t x0;
        size_t y0;
        size_t x1;
        size_t y1;
        size_t done_index;
        std::vector<unsigned char> png;
    };

    job_state_t state;
    size_t width;
    size_t height;
    size_t tiles_done;
    size_t tiles_total;
    std::vector<tile_t> tiles;
};

class job_manager_t
{
    public:
    job_manager_t();
    void set_max_concurrent_renders(size_t max_concurrent);
    size_t get_max_concurrent_renders() const;
    size_t get_active_render_count() const;

    std::string create(const common::render_request_t &request,
                       const std::string &scene_name,
                       const std::string &workspace_id,
                       const std::string &cleanup_scene_path);
    bool snapshot(const std::string &id, job_snapshot_t &out);
    bool image(const std::string &id,
               std::vector<unsigned char> &out,
               bool allow_partial,
               const xtcore::tonemapping::settings_t &tm_settings);
    bool image_delta(const std::string &id,
                     size_t since_done,
                     size_t max_tiles,
                     const xtcore::tonemapping::settings_t &tm_settings,
                     job_image_delta_t &out);
    bool image_export(const std::string &id,
                      const std::string &format,
                      std::vector<unsigned char> &out,
                      std::string &mime_type,
                      std::string &extension);
    bool photons(const std::string &id,
                 std::vector<common::render_result_t::point3_t> &diffuse_out,
                 std::vector<common::render_result_t::point3_t> &caustic_out,
                 size_t limit_per_set);

    private:
    struct job_t
    {
        mutable std::mutex mut;
        std::string id;
        std::string workspace_id;
        std::string scene;
        std::string integrator;
        std::atomic<job_state_t> state;
        std::atomic<size_t> tiles_done;
        std::atomic<size_t> tiles_total;
        std::string error;
        double elapsed_ms;
        std::vector<unsigned char> image_png;
        nimg::Pixmap final_fb;
        std::vector<unsigned char> image_exr;
        std::vector<unsigned char> image_hdr;
        std::vector<unsigned char> image_jpg;
        std::vector<unsigned char> image_bmp;
        std::vector<unsigned char> image_tga;
        std::vector<unsigned char> image_raygraph_ply;
        nimg::Pixmap progressive_fb;
        struct finished_tile_t {
            job_snapshot_t::tile_rect_t rect;
            size_t done_index;
        };
        std::vector<finished_tile_t> finished_tiles;
        std::vector<common::render_result_t::point3_t> photon_diffuse_points;
        std::vector<common::render_result_t::point3_t> photon_caustic_points;
        std::vector<job_snapshot_t::tile_rect_t> active_tiles;
        bool progressive_ready;
        size_t preview_last_encoded_done;
        bool preview_last_from_final;
        xtcore::tonemapping::operator_t preview_last_tm_op;
        float preview_last_tm_exposure;
        float preview_last_tm_white_point;
        float preview_last_tm_mantiuk_contrast;
        float preview_last_tm_mantiuk_saturation;
        float preview_last_tm_mantiuk_detail;
        std::vector<unsigned char> preview_png_cache;
        common::render_request_t request;
        std::string cleanup_scene_path;

        job_t();
    };

    struct evicted_job_t
    {
        std::string id;
        std::string workspace_id;
        std::string scene;
        std::string integrator;
        job_state_t state;
        std::string error;
        double elapsed_ms;
        size_t width;
        size_t height;
        std::string png_path;
    };

    void run(const std::shared_ptr<job_t> &job);
    std::shared_ptr<job_t> get_job(const std::string &id);
    void on_job_finished(const std::string &id);
    void prune_completed_jobs_locked();
    void cache_evicted_job_locked(const std::shared_ptr<job_t> &job);
    void prune_evicted_jobs_locked();

    mutable std::mutex jobs_mut;
    std::map<std::string, std::shared_ptr<job_t> > jobs;
    std::deque<std::string> completed_job_order;
    size_t max_completed_jobs;
    std::map<std::string, evicted_job_t> evicted_jobs;
    std::deque<std::string> evicted_job_order;
    size_t max_evicted_jobs;
    std::atomic<unsigned long long> next_id;

    // Limit concurrent renders to avoid unbounded oversubscription.
    mutable std::mutex render_slots_mut;
    std::condition_variable render_slots_cv;
    size_t max_concurrent_renders;
    size_t active_renders;
};

} /* namespace web */
} /* namespace frontend */
} /* namespace xtracer */

#endif /* XTRACER_FRONTEND_WEB_JOB_MANAGER_H_INCLUDED */
