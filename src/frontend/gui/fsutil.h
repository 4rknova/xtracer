#ifndef XT_FSUTIL_H_INCLUDED
#define XT_FSUTIL_H_INCLUDED

#include <string>
#include <vector>

namespace util {
    namespace filesystem {

enum FS_ENTRY_TYPE {
      FS_ENTRY_TYPE_UNKNOWN
    , FS_ENTRY_TYPE_FILE
    , FS_ENTRY_TYPE_DIRECTORY
};

struct fs_entry_t {
    std::string   path;
    std::string   name;
    FS_ENTRY_TYPE type;

    fs_entry_t()
        : type(FS_ENTRY_TYPE_UNKNOWN)
    {}
};

typedef std::vector<fs_entry_t> fsvec;

int ls(fsvec &fsv, const char *path);
int is_file(const char *path);

    } /* namespace filesystem */
} /* namespace util */

#endif /* XT_FSUTIL_H_INCLUDED */
