#ifndef XTRACER_FRONTEND_WEB_GALLERY_MANAGER_H_INCLUDED
#define XTRACER_FRONTEND_WEB_GALLERY_MANAGER_H_INCLUDED

#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace xtracer {
namespace frontend {
namespace web {

struct gallery_entry_meta_t {
    std::string id;
    std::string scene;
    std::string workspace_id;
    std::string integrator;
    std::string render_mode;
    size_t width;
    size_t height;
    size_t samples;
    size_t aa;
    size_t rdepth;
    size_t threads;
    size_t tile_size;
    double elapsed_ms;
    long long created_at_ms;
    size_t pass_count;
    // Default tonemapping settings for gallery preview
    std::string tm_op;
    float tm_exposure;
    float tm_white_point;
    float tm_mantiuk_contrast;
    float tm_mantiuk_saturation;
    float tm_mantiuk_detail;

    gallery_entry_meta_t()
        : width(0), height(0), samples(0), aa(0), rdepth(0)
        , threads(0), tile_size(0), elapsed_ms(0.0), created_at_ms(0)
        , pass_count(0)
        , tm_op("aces")
        , tm_exposure(1.0f)
        , tm_white_point(1.0f)
        , tm_mantiuk_contrast(0.1f)
        , tm_mantiuk_saturation(0.8f)
        , tm_mantiuk_detail(1.0f)
    {}
};

class gallery_manager_t {
public:
    gallery_manager_t();
    bool init(const std::string &gallery_dir);
    bool is_initialized() const;

    // Create a new gallery entry (call when render first produces output)
    bool create_entry(const gallery_entry_meta_t &meta,
                      const std::vector<unsigned char> &exr);

    // Update render.exr and elapsed for an existing entry
    bool update_render(const std::string &id,
                       const std::vector<unsigned char> &exr,
                       double elapsed_ms);

    // Save a progressive pass image and increment pass_count
    bool save_pass(const std::string &id,
                   size_t pass_index,
                   const std::vector<unsigned char> &exr,
                   double elapsed_ms);

    // List all entry IDs, sorted newest first
    std::vector<std::string> list_entry_ids() const;

    // Serialize metadata as JSON for a single entry
    bool get_meta_json(const std::string &id, std::string &out) const;

    // Read the main render image (EXR bytes, raw float)
    bool get_image_exr(const std::string &id, std::vector<unsigned char> &out) const;

    // Read a specific pass image (EXR bytes, raw float)
    bool get_pass_image_exr(const std::string &id, size_t pass_index,
                            std::vector<unsigned char> &out) const;

    // Read metadata as a struct (in-memory or parsed from disk)
    bool get_meta_struct(const std::string &id, gallery_entry_meta_t &out) const;

    // Remove an entry from disk
    bool delete_entry(const std::string &id);

private:
    std::string entry_dir(const std::string &id) const;
    bool write_meta_locked(const std::string &id);
    bool write_file(const std::string &path, const void *data, size_t size);
    bool read_file(const std::string &path, std::vector<unsigned char> &out) const;

    std::string gallery_dir_;
    bool initialized_;
    mutable std::mutex mut_;

    // In-memory state for entries created or updated this session
    std::map<std::string, gallery_entry_meta_t> entries_;
};

} /* namespace web */
} /* namespace frontend */
} /* namespace xtracer */

#endif /* XTRACER_FRONTEND_WEB_GALLERY_MANAGER_H_INCLUDED */
