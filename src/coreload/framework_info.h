#ifndef FRAMEWORK_INFO_H_
#define FRAMEWORK_INFO_H_

#include "libhost.h"

namespace coreload
{
    struct framework_info
    {
        framework_info(pal::string_t name, pal::string_t path, fx_ver_t version)
            : name(name)
            , path(path)
            , version(version) { }
        static void get_all_framework_infos(
            const pal::string_t& own_dir,
            const pal::string_t& fx_name,
            std::vector<framework_info>* framework_infos);

        static bool print_all_frameworks(const pal::string_t& own_dir, const pal::string_t& leading_whitespace);

        pal::string_t name;
        pal::string_t path;
        fx_ver_t version;
    };

}
#endif // FRAMEWORK_INFO_H_
