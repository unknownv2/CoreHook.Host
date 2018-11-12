#ifndef LIBHOST_H
#define LIBHOST_H

#include "pal.h"
#include "host_startup_info.h"
#include "runtime_config.h"
#include "fx_definition.h"
#include "fx_ver.h"
#include "logging.h"
#include <stdint.h>

namespace coreload {

    enum HostMode {
        INVALID,
        MUXER,
        STANDALONE,
        SPLITFX
    };

    struct Strarr {
        // DO NOT modify this struct. It is used in a layout
        // dependent manner. Create another for your use.
        size_t len;
        const pal::char_t** arr;
    };

    struct HostInterface {
        size_t version_lo;                // Just assign sizeof() to this field.
        size_t version_hi;                // Breaking changes to the layout -- increment HOST_INTERFACE_LAYOUT_VERSION
        Strarr config_keys;
        Strarr config_values;
        const pal::char_t* fx_dir;
        const pal::char_t* fx_name;
        const pal::char_t* deps_file;
        size_t is_framework_dependent;
        Strarr probe_paths;
        size_t patch_roll_forward;
        size_t prerelease_roll_forward;
        size_t host_mode;
        const pal::char_t* tfm;
        const pal::char_t* additional_deps_serialized;
        const pal::char_t* fx_ver;
        Strarr fx_names;
        Strarr fx_dirs;
        Strarr fx_requested_versions;
        Strarr fx_found_versions;
        const pal::char_t* host_command;
        const pal::char_t* host_info_host_path;
        const pal::char_t* host_info_dotnet_root;
        const pal::char_t* host_info_app_path;
        // !! WARNING / WARNING / WARNING / WARNING / WARNING / WARNING / WARNING / WARNING / WARNING
        // !! 1. Only append to this structure to maintain compat.
        // !! 2. Any nested structs should not use compiler specific padding (pack with _HOST_INTERFACE_PACK)
        // !! 3. Do not take address of the fields of this struct or be prepared to deal with unaligned accesses.
        // !! 4. Must be POD types; only use non-const size_t and pointer types; no access modifiers.
        // !! 5. Do not reorder fields or change any existing field types.
        // !! 6. Add static asserts for fields you add.
    };
#define HOST_INTERFACE_LAYOUT_VERSION_HI 0x16041101 // YYMMDD:nn always increases when layout breaks compat.
#define HOST_INTERFACE_LAYOUT_VERSION_LO sizeof(HostInterface)
    class HostPolicy {
    public:
        std::vector<std::vector<char>> cfg_keys;
        std::vector<std::vector<char>> cfg_values;
        pal::string_t deps_file;
        pal::string_t additional_deps_serialized;
        std::vector<pal::string_t> probe_paths;
        fx_definition_vector_t fx_definitions;
        pal::string_t tfm;
        HostMode host_mode;
        bool patch_roll_forward;
        bool prerelease_roll_forward;
        bool is_framework_dependent;
        pal::string_t host_command;
        HostStartupInfo host_info;

        HostPolicy() {}

        static bool init(HostInterface* input, HostPolicy* init) {
            // Check if there are any breaking changes.
            if (input->version_hi != HOST_INTERFACE_LAYOUT_VERSION_HI)
            {
                logging::logger::instance().error(_X("The version of the data layout used to initialize %s is [0x%04x]; expected version [0x%04x]"), LIBHOSTPOLICY_NAME, input->version_hi, HOST_INTERFACE_LAYOUT_VERSION_HI);
                return false;
            }

            logging::logger::instance().verbose(_X("Reading from host interface version: [0x%04x:%d] to initialize policy version: [0x%04x:%d]"), input->version_hi, input->version_lo, HOST_INTERFACE_LAYOUT_VERSION_HI, HOST_INTERFACE_LAYOUT_VERSION_LO);

            //This check is to ensure is an old hostfxr can still load new hostpolicy.
            //We should not read garbage due to potentially shorter struct size

            pal::string_t fx_requested_ver;

            if (input->version_lo >= offsetof(HostInterface, host_mode) + sizeof(input->host_mode))
            {
                make_clrstr_arr(input->config_keys.len, input->config_keys.arr, &init->cfg_keys);
                make_clrstr_arr(input->config_values.len, input->config_values.arr, &init->cfg_values);

                init->deps_file = input->deps_file;
                init->is_framework_dependent = input->is_framework_dependent;

                make_palstr_arr(input->probe_paths.len, input->probe_paths.arr, &init->probe_paths);

                init->patch_roll_forward = input->patch_roll_forward;
                init->prerelease_roll_forward = input->prerelease_roll_forward;
                init->host_mode = (HostMode)input->host_mode;
            }
            else
            {
                logging::logger::instance().error(_X("The size of the data layout used to initialize %s is %d; expected at least %d"), LIBHOSTPOLICY_NAME, input->version_lo,
                    offsetof(HostInterface, host_mode) + sizeof(input->host_mode));
            }

            //An old hostfxr may not provide these fields.
            //The version_lo (sizeof) the old hostfxr saw at build time will be
            //smaller and we should not attempt to read the fields in that case.
            if (input->version_lo >= offsetof(HostInterface, tfm) + sizeof(input->tfm))
            {
                init->tfm = input->tfm;
            }

            if (input->version_lo >= offsetof(HostInterface, fx_ver) + sizeof(input->fx_ver))
            {
                init->additional_deps_serialized = input->additional_deps_serialized;
                fx_requested_ver = input->fx_ver;
            }

            int fx_count = 0;
            if (input->version_lo >= offsetof(HostInterface, fx_names) + sizeof(input->fx_names))
            {
                int fx_count = input->fx_names.len;
                assert(fx_count > 0);
                assert(fx_count == input->fx_dirs.len);
                assert(fx_count == input->fx_requested_versions.len);
                assert(fx_count == input->fx_found_versions.len);

                std::vector<pal::string_t> fx_names;
                std::vector<pal::string_t> fx_dirs;
                std::vector<pal::string_t> fx_requested_versions;
                std::vector<pal::string_t> fx_found_versions;

                make_palstr_arr(input->fx_names.len, input->fx_names.arr, &fx_names);
                make_palstr_arr(input->fx_dirs.len, input->fx_dirs.arr, &fx_dirs);
                make_palstr_arr(input->fx_requested_versions.len, input->fx_requested_versions.arr, &fx_requested_versions);
                make_palstr_arr(input->fx_found_versions.len, input->fx_found_versions.arr, &fx_found_versions);

                init->fx_definitions.reserve(fx_count);
                for (int i = 0; i < fx_count; ++i)
                {
                    auto fx = new FxDefinition(fx_names[i], fx_dirs[i], fx_requested_versions[i], fx_found_versions[i]);
                    init->fx_definitions.push_back(std::unique_ptr<FxDefinition>(fx));
                }
            }
            else
            {
                // Backward compat; create the fx_definitions[0] and [1] from the previous information
                init->fx_definitions.reserve(2);

                auto fx = new FxDefinition();
                init->fx_definitions.push_back(std::unique_ptr<FxDefinition>(fx));

                if (init->is_framework_dependent)
                {
                    pal::string_t fx_dir = input->fx_dir;
                    pal::string_t fx_name = input->fx_name;

                    // The found_ver was not passed previously, so obtain that from fx_dir
                    pal::string_t fx_found_ver;
                    int index = fx_dir.rfind(DIR_SEPARATOR);
                    if (index != pal::string_t::npos)
                    {
                        fx_found_ver = fx_dir.substr(index + 1);
                    }

                    fx = new FxDefinition(fx_name, fx_dir, fx_requested_ver, fx_found_ver);
                    init->fx_definitions.push_back(std::unique_ptr<FxDefinition>(fx));
                }
            }

            if (input->version_lo >= offsetof(HostInterface, host_command) + sizeof(input->host_command))
            {
                init->host_command = input->host_command;
            }

            if (input->version_lo >= offsetof(HostInterface, host_info_host_path) + sizeof(input->host_info_host_path))
            {
                init->host_info.host_path = input->host_info_host_path;
                init->host_info.dotnet_root = input->host_info_dotnet_root;
                init->host_info.app_path = input->host_info_app_path;
                // For the backwards compat case, this will be later initialized with argv[0]
            }

            return true;
        }
private:
    static void make_palstr_arr(int argc, const pal::char_t** argv, std::vector<pal::string_t>* out)
    {
        out->reserve(argc);
        for (int i = 0; i < argc; ++i)
        {
            out->push_back(argv[i]);
        }
    }

    static void make_clrstr_arr(int argc, const pal::char_t** argv, std::vector<std::vector<char>>* out)
    {
        out->resize(argc);
        for (int i = 0; i < argc; ++i)
        {
            pal::pal_clrstring(pal::string_t(argv[i]), &(*out)[i]);
        }
    }
    };
} // namespace coreload
#endif // LIBHOST_H