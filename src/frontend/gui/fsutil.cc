#include <cstring>
#include <algorithm>

#define TINYFILES_IMPL
#include <tinyfiles/tinyfiles.h>
#include "fsutil.h"

namespace util {
    namespace filesystem {

int ls(fsvec &fsv, const char *path)
{
    fsv.clear();

    tfDIR dir;
    int is_valid = tfDirOpen(&dir, path);

    if (!is_valid) return 0;

    while (dir.has_next) {
        tfFILE file;
        tfReadFile(&dir, &file);
        fs_entry_t entry;
        fsv.push_back(entry);
        fs_entry_t *p = &(fsv.back());
        p->name = file.name;
        p->path = file.path;

        FS_ENTRY_TYPE type = FS_ENTRY_TYPE_UNKNOWN;

        if (file.is_dir) {
            std::string name = file.name;
                 if (name.compare("." ) == 0) type = FS_ENTRY_TYPE_SPECIAL_DIR_CURRENT;
            else if (name.compare("..") == 0) type = FS_ENTRY_TYPE_SPECIAL_DIR_PARENT;
            else                              type = FS_ENTRY_TYPE_DIRECTORY;
        }
        else type = FS_ENTRY_TYPE_FILE;

        p->type = type;

        tfDirNext(&dir);
    }
    tfDirClose(&dir);

    std::sort(fsv.begin(), fsv.end(),
        [](const fs_entry_t &a, const fs_entry_t &b) -> bool {
            return a.type < b.type;
        }
    );

    return 1;
}

bool has_extension(fs_entry_t *entry, const char *extension)
{
    if (!entry || !extension) return false;

    std::string src = entry->name;
    size_t      idx = src.find_last_of('.');

    if (  (idx == std::string::npos)
       || (src.length() - (++idx)) != strlen(extension)
    ) return false;

    std::string ext = src.substr(idx);
    std::transform(ext.begin(), ext.end(), ext.begin(), tolower);

    return ext.compare(extension) == 0;
}

    } /* namespace filesystem */
} /* namespace util */
