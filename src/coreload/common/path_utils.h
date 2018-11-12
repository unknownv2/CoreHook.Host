#ifndef  PATH_UTILS
#define PATH_UTILS

#include "pal.h"

namespace coreload {

    struct host_option {

    };

#define _STRINGIFY(s) _X(s)

    void append_path(pal::string_t* path1, const pal::char_t* path2);
    pal::string_t get_filename(const pal::string_t& path);
    pal::string_t get_replaced_char(const pal::string_t& path, pal::char_t match, pal::char_t repl);
    pal::string_t get_directory(const pal::string_t& path);
    void remove_trailing_dir_seperator(pal::string_t* dir);
    void replace_char(pal::string_t* path, pal::char_t match, pal::char_t repl);
    bool try_stou(const pal::string_t& str, unsigned* num);
    pal::string_t strip_file_ext(const pal::string_t& path);
    pal::string_t strip_executable_ext(const pal::string_t& filename);
}
#endif // ! PATH_UTILS
