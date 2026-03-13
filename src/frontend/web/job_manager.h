#ifndef XTRACER_FRONTEND_WEB_JOB_MANAGER_H_INCLUDED
#define XTRACER_FRONTEND_WEB_JOB_MANAGER_H_INCLUDED

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <atomic>

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

class job_manager_t
{
    public:
    job_manager_t();

    std::string create(const common::render_request_t &request, const std::string &scene_name);
    bool snapshot(const std::string &id, job_snapshot_t &out);
    bool image(const std::string &id,
               std::vector<unsigned char> &out,
               bool allow_partial,
               const xtcore::tonemapping::settings_t &tm_settings);
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

        job_t();
    };

    void run(const std::shared_ptr<job_t> &job);
    std::shared_ptr<job_t> get_job(const std::string &id);

    mutable std::mutex jobs_mut;
    std::map<std::string, std::shared_ptr<job_t> > jobs;
    std::atomic<unsigned long long> next_id;

    // Global render lock keeps job execution deterministic and avoids
    // oversubscribing the process when each render already uses OpenMP.
    std::mutex render_mut;
};

} /* namespace web */
} /* namespace frontend */
} /* namespace xtracer */

#endif /* XTRACER_FRONTEND_WEB_JOB_MANAGER_H_INCLUDED */
