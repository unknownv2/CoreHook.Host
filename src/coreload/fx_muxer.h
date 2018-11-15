#pragma once
#include "libhost.h"
namespace coreload {

    class corehost_init_t;
    class runtime_config_t;
    class fx_definition_t;
    struct fx_ver_t;
    struct host_startup_info_t;

    int read_config(
        fx_definition_t& app,
        const pal::string_t& app_candidate,
        pal::string_t& runtime_config);

    class fx_muxer_t
    {
    public:
        static int read_config_and_execute(
            const arguments_t& arguments,
            const host_startup_info_t& host_info,
            const pal::string_t& app_candidate,
            host_mode_t mode);
    private:

        static bool resolve_hostpolicy_dir(
            host_mode_t mode,
            const pal::string_t& dotnet_root,
            const fx_definition_vector_t& fx_definitions,
            const pal::string_t& app_candidate,
            const pal::string_t& specified_deps_file,
            const pal::string_t& specified_fx_version,
            const std::vector<pal::string_t>& probe_realpaths,
            pal::string_t* impl_dir);
        static fx_ver_t resolve_framework_version(
            const std::vector<fx_ver_t>& version_list,
            const pal::string_t& fx_ver,
            const fx_ver_t& specified,
            bool patch_roll_fwd,
            roll_fwd_on_no_candidate_fx_option roll_fwd_on_no_candidate_fx);
        static fx_definition_t* resolve_fx(
            host_mode_t mode,
            const runtime_config_t& config,
            const pal::string_t& dotnet_dir,
            const pal::string_t& specified_fx_version);
    };

}