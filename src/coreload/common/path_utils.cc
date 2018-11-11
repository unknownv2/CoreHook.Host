
#include "path_utils.h"

namespace coreload {

    pal::string_t get_filename(const pal::string_t& path) {

        if (path.empty()) {
            return path;
        }
        auto name_pos = path.find_last_of(DIR_SEPARATOR);
        if (name_pos == pal::string_t::npos) {
            return path;
        }
        return path.substr(name_pos + 1);
    }

    void append_path(pal::string_t* path1, const pal::char_t* path2) {

    }
}