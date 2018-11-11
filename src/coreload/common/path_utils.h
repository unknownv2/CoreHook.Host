#ifndef  PATH_UTILS
#define PATH_UTILS

#include "pal.h"

namespace coreload {

    struct host_option {

    };

    void append_path(pal::string_t* path1, const pal::char_t* path2);
    pal::string_t get_filename(const pal::string_t& path);
}
#endif // ! PATH_UTILS
