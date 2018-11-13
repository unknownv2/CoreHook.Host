#ifndef ARGUMENTS_H
#define ARGUMENTS_H

#include "utils.h"
#include "pal.h"
#include "logging.h"
#include "trace.h"
#include "deps_format.h"

namespace coreload {
    struct probe_config_t
    {
        pal::string_t probe_dir;
        bool patch_roll_fwd;
        const deps_json_t* probe_deps_json;
        int fx_level;

        bool only_runtime_assets;
        bool only_serviceable_assets;

        bool probe_publish_dir;

        void print() const
        {
            trace::verbose(_X("probe_config_t: probe=[%s] deps-dir-probe=[%d]"),
                probe_dir.c_str(), probe_publish_dir);
        }

        probe_config_t(
            const pal::string_t& probe_dir,
            const deps_json_t* probe_deps_json,
            int fx_level,
            bool only_serviceable_assets,
            bool only_runtime_assets,
            bool probe_publish_dir)
            : probe_dir(probe_dir)
            , probe_deps_json(probe_deps_json)
            , fx_level(fx_level)
            , only_serviceable_assets(only_serviceable_assets)
            , only_runtime_assets(only_runtime_assets)
            , probe_publish_dir(probe_publish_dir)
        {
        }

        bool is_lookup() const
        {
            return (probe_deps_json == nullptr) &&
                !only_runtime_assets &&
                !only_serviceable_assets &&
                !probe_publish_dir;
        }

        bool is_fx() const
        {
            return (probe_deps_json != nullptr);
        }

        bool is_app() const
        {
            return probe_publish_dir;
        }

        static probe_config_t svc_ni(const pal::string_t& dir)
        {
            return probe_config_t(dir, nullptr, -1, true, true, false);
        }

        static probe_config_t svc(const pal::string_t& dir)
        {
            return probe_config_t(dir, nullptr, -1, true, false, false);
        }

        static probe_config_t fx(const pal::string_t& dir, const deps_json_t* deps, int fx_level)
        {
            return probe_config_t(dir, deps, fx_level, false, false, false);
        }

        static probe_config_t lookup(const pal::string_t& dir)
        {
            return probe_config_t(dir, nullptr, -1, false, false, false);
        }

        static probe_config_t published_deps_dir()
        {
            return probe_config_t(_X(""), nullptr, 0, false, false, true);
        }
    };
    struct arguments_t {
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

        arguments_t();
    };
}
#endif // ARGUMENTS_H
