#ifndef XT_FSUTIL_H_INCLUDED
#define XT_FSUTIL_H_INCLUDED

#include <string>
#include <vector>

namespace util {
    namespace filesystem {

const char PATH_DELIMITER = '/';

enum FS_ENTRY_TYPE
{
      FS_ENTRY_TYPE_UNKNOWN             = 0x01
    , FS_ENTRY_TYPE_SPECIAL_DIR_CURRENT = 0x02
    , FS_ENTRY_TYPE_SPECIAL_DIR_PARENT  = 0x04
    , FS_ENTRY_TYPE_DIRECTORY           = 0x08
    , FS_ENTRY_TYPE_FILE                = 0x10
};

struct fs_entry_t
{
    std::string   path;
    std::string   name;
    FS_ENTRY_TYPE type;
};

typedef std::vector<fs_entry_t> fsvec;

void decompose_path(const char *path, std::string &base, std::string &file, const char delim = PATH_DELIMITER);
int  ls(fsvec &fsv, const char *path);
bool file_exists(const char *filepath);
bool has_extension(fs_entry_t *entry, const char *extension);

    } /* namespace filesystem */
} /* namespace util */

#endif /* XT_FSUTIL_H_INCLUDED */
