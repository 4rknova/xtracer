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
    std::string id;
    std::string scene;
    std::string integrator;
    job_state_t state;
    float progress;
    std::string error;
    double elapsed_ms;
    bool has_image;
};

class job_manager_t
{
    public:
    job_manager_t();

    std::string create(const common::render_request_t &request, const std::string &scene_name);
    bool snapshot(const std::string &id, job_snapshot_t &out);
    bool image(const std::string &id, std::vector<unsigned char> &out, bool allow_partial);

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
        nimg::Pixmap progressive_fb;
        bool progressive_ready;
        size_t last_encoded_done;
        std::vector<unsigned char> progressive_png_cache;
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
