#ifndef XTRACER_FRONTEND_WEB_JOB_MANAGER_H_INCLUDED
#define XTRACER_FRONTEND_WEB_JOB_MANAGER_H_INCLUDED

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <deque>
#include <atomic>
#include <condition_variable>
#include <chrono>

#include <frontend/common/render_service.h>
#include <nimg/pixmap.h>
#include <xtcore/tonemapping/tonemapping.h>
#include "gallery_manager.h"

namespace xtracer {
namespace frontend {
namespace web {

enum job_state_t
{
    JOB_QUEUED = 0,
    JOB_PREPARING,
    JOB_RUNNING,
    JOB_DONE,
    JOB_ABORTED,
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
    std::string render_mode;
    job_state_t state;
    float progress;
    std::string error;
    double elapsed_ms;
    bool has_image;
    size_t width;
    size_t height;
    size_t threads;
    size_t tiles_done;
    size_t tiles_total;
    size_t pass_current;
    size_t pass_total;
    int queue_index;
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
    ~job_manager_t();
    void set_gallery_manager(gallery_manager_t *gm);
    void set_max_concurrent_renders(size_t max_concurrent);
    void set_render_thread_budget(size_t max_threads);
    size_t get_max_concurrent_renders() const;
    size_t get_active_render_count() const;

    std::string create(const common::render_request_t &request,
                       const std::string &scene_name,
                       const std::string &workspace_id,
                       const std::string &owner_client_id,
                       const std::string &cleanup_scene_path,
                       const xtcore::tonemapping::settings_t &initial_tm_settings = xtcore::tonemapping::settings_t());
    bool snapshot(const std::string &id, job_snapshot_t &out);
    bool image(const std::string &id,
               std::vector<unsigned char> &out,
               bool allow_partial,
               const xtcore::tonemapping::settings_t &tm_settings,
               bool post_filters_enabled,
               const std::string &post_filters);
    // Returns the full progressive frame as raw RGBA u8 (sRGB, alpha=255) with
    // tonemapping applied — same pixel pipeline as live XTDR tile pushes.
    // Populates width_out / height_out / tiles_done_out / tiles_total_out.
    // Returns false if the job has no partial data yet.
    bool image_rgba(const std::string &id,
                    std::vector<unsigned char> &rgba_out,
                    size_t &width_out,
                    size_t &height_out,
                    size_t &tiles_done_out,
                    size_t &tiles_total_out);
    bool image_delta(const std::string &id,
                     size_t since_done,
                     size_t max_tiles,
                     const xtcore::tonemapping::settings_t &tm_settings,
                     bool post_filters_enabled,
                     const std::string &post_filters,
                     job_image_delta_t &out);
    bool image_export(const std::string &id,
                      const std::string &format,
                      std::vector<unsigned char> &out,
                      std::string &mime_type,
                      std::string &extension,
                      bool post_filters_enabled,
                      const std::string &post_filters);
    bool photons(const std::string &id,
                 std::vector<common::render_result_t::point3_t> &diffuse_out,
                 std::vector<common::render_result_t::point3_t> &caustic_out,
                 size_t limit_per_set);
    bool abort(const std::string &id);
    bool set_live_tm_settings(const std::string &id, const xtcore::tonemapping::settings_t &tm);
    bool belongs_to_client(const std::string &id, const std::string &client_id);
    bool move_queue_up(const std::string &id);
    bool move_queue_down(const std::string &id);
    bool list_active(std::vector<job_snapshot_t> &out);

    void set_push_callback(
        std::function<void(const std::string &job_id,
                           const job_snapshot_t &snap,
                           const std::vector<unsigned char> &tile_xtdr)> cb);

    private:
    struct job_t
    {
        mutable std::mutex mut;
        std::string id;
        std::string workspace_id;
        std::string owner_client_id;
        std::string scene;
        std::string integrator;
        std::atomic<job_state_t> state;
        std::atomic<bool> cancel_requested;
        std::atomic<size_t> tiles_done;
        std::atomic<size_t> tiles_total;
        std::string error;
        double elapsed_ms;
        std::chrono::steady_clock::time_point started_at;
        bool has_started;
        std::vector<unsigned char> image_png;
        nimg::Pixmap final_fb;
        std::vector<unsigned char> image_exr;
        std::vector<unsigned char> image_hdr;
        std::vector<unsigned char> image_jpg;
        std::vector<unsigned char> image_bmp;
        std::vector<unsigned char> image_tga;
        nimg::Pixmap progressive_fb;
        struct finished_tile_t {
            job_snapshot_t::tile_rect_t rect;
            size_t done_index;
        };
        std::vector<finished_tile_t> finished_tiles;
        std::vector<common::render_result_t::point3_t> photon_diffuse_points;
        std::vector<common::render_result_t::point3_t> photon_caustic_points;
        struct active_tile_key_t {
            size_t x0;
            size_t y0;
            size_t x1;
            size_t y1;

            bool operator<(const active_tile_key_t &rhs) const
            {
                if (x0 != rhs.x0) return x0 < rhs.x0;
                if (y0 != rhs.y0) return y0 < rhs.y0;
                if (x1 != rhs.x1) return x1 < rhs.x1;
                return y1 < rhs.y1;
            }
        };
        std::vector<job_snapshot_t::tile_rect_t> active_tiles;
        std::map<active_tile_key_t, size_t> active_tile_index;
        bool progressive_ready;
        size_t preview_last_encoded_done;
        bool preview_last_from_final;
        xtcore::tonemapping::operator_t preview_last_tm_op;
        float preview_last_tm_exposure;
        float preview_last_tm_white_point;
        float preview_last_tm_mantiuk_contrast;
        float preview_last_tm_mantiuk_saturation;
        float preview_last_tm_mantiuk_detail;
        bool preview_last_post_filters_enabled;
        std::string preview_last_post_filters;
        std::vector<unsigned char> preview_png_cache;
        xtcore::tonemapping::settings_t live_tm_settings;
        size_t effective_threads;
        common::render_request_t request;
        std::string cleanup_scene_path;

        job_t();
    };

    struct scheduled_job_t
    {
        std::shared_ptr<job_t> job;
        size_t granted_threads;
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
        size_t threads;
        std::string exr_path;
    };

    void run(const std::shared_ptr<job_t> &job, size_t granted_threads);
    std::shared_ptr<job_t> get_job(const std::string &id);
    void on_job_finished(const std::string &id);
    void dispatch_queued_jobs();
    void release_render_slots(size_t released_threads);
    void prune_completed_jobs_locked();
    void cache_evicted_job_locked(const std::shared_ptr<job_t> &job);
    void prune_evicted_jobs_locked();
    static job_t::active_tile_key_t make_active_tile_key(size_t x0, size_t y0, size_t x1, size_t y1);
    static job_t::active_tile_key_t make_active_tile_key(const xtcore::render::tile_t *tile,
                                                         const common::progress_tile_update_t *upd);
    static void add_active_tile(job_t &job, const job_t::active_tile_key_t &key);
    static void remove_active_tile(job_t &job, const job_t::active_tile_key_t &key);

    std::function<void(const std::string &,
                       const job_snapshot_t &,
                       const std::vector<unsigned char> &)> push_callback_;

    gallery_manager_t *gallery_manager_;
    mutable std::mutex jobs_mut;
    std::map<std::string, std::shared_ptr<job_t> > jobs;
    std::deque<std::string> completed_job_order;
    size_t max_completed_jobs;
    std::map<std::string, evicted_job_t> evicted_jobs;
    std::deque<std::string> evicted_job_order;
    size_t max_evicted_jobs;
    size_t max_queued_jobs;
    std::atomic<unsigned long long> next_id;

    // Limit concurrent renders to avoid unbounded oversubscription.
    mutable std::mutex render_slots_mut;
    std::condition_variable render_slots_cv;
    size_t max_concurrent_renders;
    size_t active_renders;
    size_t render_thread_budget;
    size_t active_render_threads;
    std::deque<std::string> queued_job_order;
    std::atomic<bool> stopping_;
};

} /* namespace web */
} /* namespace frontend */
} /* namespace xtracer */

#endif /* XTRACER_FRONTEND_WEB_JOB_MANAGER_H_INCLUDED */
