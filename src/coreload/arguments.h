#ifndef ARGUMENTS_H
#define ARGUMENTS_H

#include "path_utils.h"
#include "pal.h"
#include "logging.h"
#include "deps_format.h"

namespace coreload {
    struct Arguments {
        pal::string_t host_path;
        pal::string_t app_root;
        pal::string_t deps_path;
        pal::string_t core_servicing;
        std::vector<pal::string_t> probe_paths;
        pal::string_t managed_application;
        std::vector<pal::string_t> global_shared_stores;
        pal::string_t dotnet_shared_store;
        std::vector<pal::string_t> env_shared_store;
        int app_argc;
        const pal::char_t** app_argv;

        Arguments();
    };
}
#endif // ARGUMENTS_H
