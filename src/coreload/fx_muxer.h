#ifndef FX_MUXER_H_
#define FX_MUXER_H_

#include "libhost.h"
#include "arguments.h"
#include "coreclr.h"

namespace coreload
{
    const int Max_Framework_Resolve_Retries = 100;

    class corehost_init_t;
    class runtime_config_t;
    class fx_definition_t;
    struct fx_ver_t;
    class host_startup_info_t;

    int read_config(
        fx_definition_t& app,
        const pal::string_t& app_candidate,
        pal::string_t& runtime_config,
        const fx_reference_t& override_settings);

    class fx_muxer_t
    {
    public:
        static int initialize_clr(
            arguments_t& arguments,
            const host_startup_info_t& host_info,
            host_mode_t mode,
            coreclr::domain_id_t& domain_id,
            coreclr::host_handle_t& host_handle);

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
        static int read_framework(
            const host_startup_info_t& host_info,
            const fx_reference_t& override_settings,
            const runtime_config_t& config,
            fx_name_to_fx_reference_map_t& newest_references,
            fx_name_to_fx_reference_map_t& oldest_references,
            fx_definition_vector_t& fx_definitions);
        static fx_definition_t* resolve_fx(
            const fx_reference_t& config,
            const pal::string_t& oldest_requested_version,
            const pal::string_t& dotnet_dir);
        static void muxer_usage(bool is_sdk_present);
        static int soft_roll_forward_helper(
            const fx_reference_t& newer,
            const fx_reference_t& older,
            bool older_is_hard_roll_forward,
            fx_name_to_fx_reference_map_t& newest_references,
            fx_name_to_fx_reference_map_t& oldest_references);
        static int soft_roll_forward(
            const fx_reference_t existing_ref,
            bool current_is_hard_roll_forward,
            fx_name_to_fx_reference_map_t& newest_references,
            fx_name_to_fx_reference_map_t& oldest_references);
        static void display_missing_framework_error(
            const pal::string_t& fx_name,
            const pal::string_t& fx_version,
            const pal::string_t& fx_dir,
            const pal::string_t& dotnet_root);
        static void display_incompatible_framework_error(
            const pal::string_t& higher,
            const fx_reference_t& lower);
        static void display_compatible_framework_trace(
            const pal::string_t& higher,
            const fx_reference_t& lower);
        static void display_retry_framework_trace(
            const fx_reference_t& fx_existing,
            const fx_reference_t& fx_new);
        static void display_summary_of_frameworks(
            const fx_definition_vector_t& fx_definitions,
            const fx_name_to_fx_reference_map_t& newest_references);
    };

} // namespace coreload

#endif // FX_MUXER_H_