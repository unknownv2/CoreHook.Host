#include <cassert>
#include "arguments.h"
#include "cpprest/json.h"
#include "deps_format.h"
#include "status_code.h"
#include "framework_info.h"
#include "fx_definition.h"
#include "fx_muxer.h"
#include "fx_ver.h"
#include "host_startup_info.h"
#include "libhost.h"
#include "pal.h"
#include "runtime_config.h"
#include "trace.h"
#include "utils.h"
#include "deps_resolver.h"
#include "coreclr.h"

namespace coreload {

    /**
    * When the framework is not found, display detailed error message
    *   about available frameworks and installation of new framework.
    */
    void handle_missing_framework_error(
        host_mode_t mode,
        const pal::string_t& fx_name,
        const pal::string_t& fx_version,
        const pal::string_t& fx_dir,
        const pal::string_t& dotnet_root)
    {
        std::vector<framework_info> framework_infos;
        pal::string_t fx_ver_dirs;
        if (fx_dir.length())
        {
            fx_ver_dirs = fx_dir;
            framework_info::get_all_framework_infos(mode, get_directory(fx_dir), fx_name, &framework_infos);
        }
        else
        {
            fx_ver_dirs = dotnet_root;
        }

        framework_info::get_all_framework_infos(mode, dotnet_root, fx_name, &framework_infos);

        // Display the error message about missing FX.
        if (fx_version.length())
        {
            trace::error(_X("The specified framework '%s', version '%s' was not found."), fx_name.c_str(), fx_version.c_str());
        }
        else
        {
            trace::error(_X("No frameworks were found."));
        }

        // Gather the list of versions installed at the shared FX location.
        bool is_print_header = true;

        for (framework_info info : framework_infos)
        {
            // Print banner only once before printing the versions
            if (is_print_header)
            {
                trace::error(_X("  - The following versions are installed:"));
                is_print_header = false;
            }

            trace::error(_X("      %s at [%s]"), info.version.as_str().c_str(), info.path.c_str());
        }
    }

    /**
    * Resolve the hostpolicy version from deps.
    *  - Scan the deps file's libraries section and find the hostpolicy version in the file.
    */
    pal::string_t resolve_hostpolicy_version_from_deps(const pal::string_t& deps_json)
    {
        trace::verbose(_X("--- Resolving %s version from deps json [%s]"), LIBHOSTPOLICY_NAME, deps_json.c_str());

        pal::string_t retval;
        if (!pal::file_exists(deps_json))
        {
            trace::verbose(_X("Dependency manifest [%s] does not exist"), deps_json.c_str());
            return retval;
        }

        pal::ifstream_t file(deps_json);
        if (!file.good())
        {
            trace::verbose(_X("Dependency manifest [%s] could not be opened"), deps_json.c_str());
            return retval;
        }

        if (skip_utf8_bom(&file))
        {
            trace::verbose(_X("UTF-8 BOM skipped while reading [%s]"), deps_json.c_str());
        }

        try
        {
            const auto root = json_value::parse(file);
            const auto& json = root.as_object();
            const auto& libraries = json.at(_X("libraries")).as_object();

            // Look up the root package instead of the "runtime" package because we can't do a full rid resolution.
            // i.e., look for "Microsoft.NETCore.DotNetHostPolicy/" followed by version.
            pal::string_t prefix = _X("Microsoft.NETCore.DotNetHostPolicy/");
            for (const auto& library : libraries)
            {
                if (starts_with(library.first, prefix, false))
                {
                    // Extract the version information that occurs after '/'
                    retval = library.first.substr(prefix.size());
                    break;
                }
            }
        }
        catch (const std::exception& je)
        {
            pal::string_t jes;
            (void)pal::utf8_palstring(je.what(), &jes);
            trace::error(_X("A JSON parsing exception occurred in [%s]: %s"), deps_json.c_str(), jes.c_str());
        }
        trace::verbose(_X("Resolved version %s from dependency manifest file [%s]"), retval.c_str(), deps_json.c_str());
        return retval;
    }

    /**
    * Given path to app binary, say app.dll or app.exe, retrieve the app.deps.json.
    */
    pal::string_t get_deps_from_app_binary(const pal::string_t& app)
    {
        assert(app.find(DIR_SEPARATOR) != pal::string_t::npos);
        assert(ends_with(app, _X(".dll"), false) || ends_with(app, _X(".exe"), false));

        // First append directory.
        pal::string_t deps_file;
        deps_file.assign(get_directory(app));

        // Then the app name and the file extension
        pal::string_t app_name = get_filename(app);
        deps_file.append(app_name, 0, app_name.find_last_of(_X(".")));
        deps_file.append(_X(".deps.json"));
        return deps_file;
    }


    /**
    * Return name of deps file for app.
    */
    pal::string_t get_deps_file(
        bool is_framework_dependent,
        const pal::string_t& app_candidate,
        const pal::string_t& specified_deps_file,
        const fx_definition_vector_t& fx_definitions
    )
    {
        if (is_framework_dependent)
        {
            // The hostpolicy is resolved from the root framework's name and location.
            pal::string_t deps_file = get_root_framework(fx_definitions).get_dir();
            if (!deps_file.empty() && deps_file.back() != DIR_SEPARATOR)
            {
                deps_file.push_back(DIR_SEPARATOR);
            }

            return deps_file + get_root_framework(fx_definitions).get_name() + _X(".deps.json");
        }
        else
        {
            // Self-contained app's hostpolicy is from specified deps or from app deps.
            return !specified_deps_file.empty() ? specified_deps_file : get_deps_from_app_binary(app_candidate);
        }
    }


    fx_ver_t fx_muxer_t::resolve_framework_version(const std::vector<fx_ver_t>& version_list,
        const pal::string_t& fx_ver,
        const fx_ver_t& specified,
        bool patch_roll_fwd,
        roll_fwd_on_no_candidate_fx_option roll_fwd_on_no_candidate_fx)
    {
        trace::verbose(_X("Attempting FX roll forward starting from [%s]"), fx_ver.c_str());

        fx_ver_t most_compatible = specified;
        if (!specified.is_prerelease())
        {
            if (roll_fwd_on_no_candidate_fx != roll_fwd_on_no_candidate_fx_option::disabled)
            {
                fx_ver_t next_lowest(-1, -1, -1);

                // Look for the least production version
                trace::verbose(_X("'Roll forward on no candidate fx' enabled with value [%d]. Looking for the least production greater than or equal to [%s]"),
                    roll_fwd_on_no_candidate_fx, fx_ver.c_str());

                for (const auto& ver : version_list)
                {
                    if (!ver.is_prerelease() && ver >= specified)
                    {
                        if (roll_fwd_on_no_candidate_fx == roll_fwd_on_no_candidate_fx_option::minor)
                        {
                            // We only want to roll forward on minor
                            if (ver.get_major() != specified.get_major())
                            {
                                continue;
                            }
                        }
                        next_lowest = (next_lowest == fx_ver_t(-1, -1, -1)) ? ver : std::min(next_lowest, ver);
                    }
                }

                if (next_lowest == fx_ver_t(-1, -1, -1))
                {
                    // Look for the least preview version
                    trace::verbose(_X("No production greater than or equal to [%s] found. Looking for the least preview greater than [%s]"),
                        fx_ver.c_str(), fx_ver.c_str());

                    for (const auto& ver : version_list)
                    {
                        if (ver.is_prerelease() && ver >= specified)
                        {
                            if (roll_fwd_on_no_candidate_fx == roll_fwd_on_no_candidate_fx_option::minor)
                            {
                                // We only want to roll forward on minor
                                if (ver.get_major() != specified.get_major())
                                {
                                    continue;
                                }
                            }
                            next_lowest = (next_lowest == fx_ver_t(-1, -1, -1)) ? ver : std::min(next_lowest, ver);
                        }
                    }
                }

                if (next_lowest == fx_ver_t(-1, -1, -1))
                {
                    trace::verbose(_X("No preview greater than or equal to [%s] found."), fx_ver.c_str());
                }
                else
                {
                    trace::verbose(_X("Found version [%s]"), next_lowest.as_str().c_str());
                    most_compatible = next_lowest;
                }
            }

            if (patch_roll_fwd)
            {
                trace::verbose(_X("Applying patch roll forward from [%s]"), most_compatible.as_str().c_str());
                for (const auto& ver : version_list)
                {
                    trace::verbose(_X("Inspecting version... [%s]"), ver.as_str().c_str());

                    if (most_compatible.is_prerelease() == ver.is_prerelease() && // prevent production from rolling forward to preview on patch
                        ver.get_major() == most_compatible.get_major() &&
                        ver.get_minor() == most_compatible.get_minor())
                    {
                        // Pick the greatest that differs only in patch.
                        most_compatible = std::max(ver, most_compatible);
                    }
                }
            }
        }
        else
        {
            for (const auto& ver : version_list)
            {
                trace::verbose(_X("Inspecting version... [%s]"), ver.as_str().c_str());

                //both production and prerelease.
                if (ver.is_prerelease() && // prevent roll forward to production.
                    ver.get_major() == specified.get_major() &&
                    ver.get_minor() == specified.get_minor() &&
                    ver.get_patch() == specified.get_patch() &&
                    ver > specified)
                {
                    // Pick the smallest prerelease that is greater than specified.
                    most_compatible = (most_compatible == specified) ? ver : std::min(ver, most_compatible);
                }
            }
        }

        return most_compatible;
    }

    fx_definition_t* fx_muxer_t::resolve_fx(
        host_mode_t mode,
        const runtime_config_t& config,
        const pal::string_t& dotnet_dir,
        const pal::string_t& specified_fx_version
    )
    {
        // If invoking using FX dotnet.exe, use own directory.
        if (mode == host_mode_t::split_fx)
        {
            return new fx_definition_t(config.get_fx_name(), dotnet_dir, pal::string_t(), pal::string_t());
        }

        assert(!config.get_fx_name().empty());
        assert(!config.get_fx_version().empty());

        trace::verbose(_X("--- Resolving FX directory, name '%s' version '%s'"),
            config.get_fx_name().c_str(), config.get_fx_version().c_str());

        const auto fx_ver = specified_fx_version.empty() ? config.get_fx_version() : specified_fx_version;
        fx_ver_t specified(-1, -1, -1);
        if (!fx_ver_t::parse(fx_ver, &specified, false))
        {
            trace::error(_X("The specified framework version '%s' could not be parsed"), fx_ver.c_str());
            return nullptr;
        }

        // Multi-level SharedFX lookup will look for the most appropriate version in several locations
        // by following the priority rank below:
        // .exe directory
        //  Global .NET directory
        // If it is not activated, then only .exe directory will be considered

        std::vector<pal::string_t> hive_dir;
        std::vector<pal::string_t> global_dirs;
        bool multilevel_lookup = multilevel_lookup_enabled();

        // dotnet_dir contains DIR_SEPARATOR appended that we need to remove.
        pal::string_t dotnet_dir_temp = dotnet_dir;
        remove_trailing_dir_seperator(&dotnet_dir_temp);

        hive_dir.push_back(dotnet_dir_temp);
        if (multilevel_lookup && pal::get_global_dotnet_dirs(&global_dirs))
        {
            for (pal::string_t dir : global_dirs)
            {
                // Avoid duplicate of dotnet_dir_temp
                if (!pal::are_paths_equal_with_normalized_casing(dir, dotnet_dir_temp))
                {
                    hive_dir.push_back(dir);
                }
            }
        }

        pal::string_t selected_fx_dir;
        pal::string_t selected_fx_version;
        fx_ver_t selected_ver(-1, -1, -1);

        for (pal::string_t dir : hive_dir)
        {
            auto fx_dir = dir;
            trace::verbose(_X("Searching FX directory in [%s]"), fx_dir.c_str());

            append_path(&fx_dir, _X("shared"));
            append_path(&fx_dir, config.get_fx_name().c_str());

            bool do_roll_forward = false;
            if (specified_fx_version.empty())
            {
                if (!specified.is_prerelease())
                {
                    // If production and no roll forward use given version.
                    do_roll_forward = (config.get_patch_roll_fwd()) || (config.get_roll_fwd_on_no_candidate_fx() != roll_fwd_on_no_candidate_fx_option::disabled);
                }
                else
                {
                    // Prerelease, but roll forward only if version doesn't exist.
                    pal::string_t ver_dir = fx_dir;
                    append_path(&ver_dir, fx_ver.c_str());
                    do_roll_forward = !pal::directory_exists(ver_dir);
                }
            }

            if (!do_roll_forward)
            {
                trace::verbose(_X("Did not roll forward because specified version='%s', patch_roll_fwd=%d, roll_fwd_on_no_candidate_fx=%d, chose [%s]"),
                    specified_fx_version.c_str(), config.get_patch_roll_fwd(), config.get_roll_fwd_on_no_candidate_fx(), fx_ver.c_str());

                append_path(&fx_dir, fx_ver.c_str());
                if (pal::directory_exists(fx_dir))
                {
                    selected_fx_dir = fx_dir;
                    selected_fx_version = fx_ver;
                    break;
                }
            }
            else
            {
                std::vector<pal::string_t> list;
                std::vector<fx_ver_t> version_list;
                pal::readdir_onlydirectories(fx_dir, &list);

                for (const auto& version : list)
                {
                    fx_ver_t ver(-1, -1, -1);
                    if (fx_ver_t::parse(version, &ver, false))
                    {
                        version_list.push_back(ver);
                    }
                }

                fx_ver_t resolved_ver = resolve_framework_version(version_list, fx_ver, specified, config.get_patch_roll_fwd(), config.get_roll_fwd_on_no_candidate_fx());

                pal::string_t resolved_ver_str = resolved_ver.as_str();
                append_path(&fx_dir, resolved_ver_str.c_str());

                if (pal::directory_exists(fx_dir))
                {
                    if (selected_ver != fx_ver_t(-1, -1, -1))
                    {
                        // Compare the previous hive_dir selection with the current hive_dir to see which one is the better match
                        std::vector<fx_ver_t> version_list;
                        version_list.push_back(resolved_ver);
                        version_list.push_back(selected_ver);
                        resolved_ver = resolve_framework_version(version_list, fx_ver, specified, config.get_patch_roll_fwd(), config.get_roll_fwd_on_no_candidate_fx());
                    }

                    if (resolved_ver != selected_ver)
                    {
                        trace::verbose(_X("Changing Selected FX version from [%s] to [%s]"), selected_fx_dir.c_str(), fx_dir.c_str());
                        selected_ver = resolved_ver;
                        selected_fx_dir = fx_dir;
                        selected_fx_version = resolved_ver_str;
                    }
                }
            }
        }

        if (selected_fx_dir.empty())
        {
            trace::error(_X("It was not possible to find any compatible framework version"));
            return nullptr;
        }

        trace::verbose(_X("Chose FX version [%s]"), selected_fx_dir.c_str());

        return new fx_definition_t(config.get_fx_name(), selected_fx_dir, fx_ver, selected_fx_version);
    }

    // Convert "path" to realpath (merging working dir if needed) and append to "realpaths" out param.
    void append_probe_realpath(const pal::string_t& path, std::vector<pal::string_t>* realpaths, const pal::string_t& tfm)
    {
        pal::string_t probe_path = path;

        if (pal::realpath(&probe_path))
        {
            realpaths->push_back(probe_path);
        }
        else
        {
            // Check if we can extrapolate |arch|<DIR_SEPARATOR>|tfm| for probing stores
            // Check for for both forward and back slashes
            pal::string_t placeholder = _X("|arch|\\|tfm|");
            auto pos_placeholder = probe_path.find(placeholder);
            if (pos_placeholder == pal::string_t::npos)
            {
                placeholder = _X("|arch|/|tfm|");
                pos_placeholder = probe_path.find(placeholder);
            }

            if (pos_placeholder != pal::string_t::npos)
            {
                pal::string_t segment = get_arch();
                segment.push_back(DIR_SEPARATOR);
                segment.append(tfm);
                probe_path.replace(pos_placeholder, placeholder.length(), segment);

                if (pal::realpath(&probe_path))
                {
                    realpaths->push_back(probe_path);
                }
                else
                {
                    trace::verbose(_X("Ignoring host interpreted additional probing path %s as it does not exist."), probe_path.c_str());
                }
            }
            else
            {
                trace::verbose(_X("Ignoring additional probing path %s as it does not exist."), probe_path.c_str());
            }
        }
    }

    int read_config(
        fx_definition_t& app,
        const pal::string_t& app_candidate,
        pal::string_t& runtime_config
    )
    {
        if (!runtime_config.empty() && !pal::realpath(&runtime_config))
        {
            trace::error(_X("The specified runtimeconfig.json [%s] does not exist"), runtime_config.c_str());
            return StatusCode::InvalidConfigFile;
        }

        pal::string_t config_file, dev_config_file;

        if (runtime_config.empty())
        {
            trace::verbose(_X("App runtimeconfig.json from [%s]"), app_candidate.c_str());
            get_runtime_config_paths_from_app(app_candidate, &config_file, &dev_config_file);
        }
        else
        {
            trace::verbose(_X("Specified runtimeconfig.json from [%s]"), runtime_config.c_str());
            get_runtime_config_paths_from_arg(runtime_config, &config_file, &dev_config_file);
        }

        app.parse_runtime_config(config_file, dev_config_file, nullptr, nullptr);
        if (!app.get_runtime_config().is_valid())
        {
            trace::error(_X("Invalid runtimeconfig.json [%s] [%s]"), app.get_runtime_config().get_path().c_str(), app.get_runtime_config().get_dev_path().c_str());
            return StatusCode::InvalidConfigFile;
        }

        return 0;
    }

#define HOST_POLICY_PKG_REL_DIR "runtimes/win-x64/native"
#define HOST_POLICY_PKG_NAME "runtimes.win-x64.Microsoft.NETCore.DotNetHostPolicy"
#define HOST_POLICY_PKG_VER "3.0.0-preview1-26822-0"
    /**
    * Given a directory and a version, find if the package relative
    *     dir under the given directory contains hostpolicy.dll
    */
    bool to_hostpolicy_package_dir(const pal::string_t& dir, const pal::string_t& version, pal::string_t* candidate)
    {
        assert(!version.empty());

        candidate->clear();

        // Ensure the relative dir contains platform directory separators.
        pal::string_t rel_dir = _STRINGIFY(HOST_POLICY_PKG_REL_DIR);
        if (DIR_SEPARATOR != '/')
        {
            replace_char(&rel_dir, '/', DIR_SEPARATOR);
        }

        // Construct the path to directory containing hostpolicy.
        pal::string_t path = dir;
        append_path(&path, _STRINGIFY(HOST_POLICY_PKG_NAME)); // package name
        append_path(&path, version.c_str());                  // package version
        append_path(&path, rel_dir.c_str());                  // relative dir containing hostpolicy library

                                                              // Check if "path" contains the required library.
        if (!library_exists_in_dir(path, LIBHOSTPOLICY_NAME, nullptr))
        {
            trace::verbose(_X("Did not find %s in directory %s"), LIBHOSTPOLICY_NAME, path.c_str());
            return false;
        }

        // "path" contains the directory containing hostpolicy library.
        *candidate = path;

        trace::verbose(_X("Found %s in directory %s"), LIBHOSTPOLICY_NAME, path.c_str());
        return true;
    }

    /**
    * Given a nuget version, detect if a serviced hostpolicy is available at
    *   platform servicing location.
    */
    bool hostpolicy_exists_in_svc(const pal::string_t& version, pal::string_t* resolved_dir)
    {
        if (version.empty())
        {
            return false;
        }

        pal::string_t svc_dir;
        pal::get_default_servicing_directory(&svc_dir);
        append_path(&svc_dir, _X("pkgs"));
        return to_hostpolicy_package_dir(svc_dir, version, resolved_dir);
    }

    bool fx_muxer_t::resolve_hostpolicy_dir(
        host_mode_t mode,
        const pal::string_t& dotnet_root,
        const fx_definition_vector_t& fx_definitions,
        const pal::string_t& app_candidate,
        const pal::string_t& specified_deps_file,
        const pal::string_t& specified_fx_version,
        const std::vector<pal::string_t>& probe_realpaths,
        pal::string_t* impl_dir)
    {

        bool is_framework_dependent = get_app(fx_definitions).get_runtime_config().get_is_framework_dependent();

        // Obtain deps file for the given configuration.
        pal::string_t resolved_deps = get_deps_file(is_framework_dependent, app_candidate, specified_deps_file, fx_definitions);

        // Resolve hostpolicy version out of the deps file.
        pal::string_t version = resolve_hostpolicy_version_from_deps(resolved_deps);
        if (trace::is_enabled() && version.empty() && pal::file_exists(resolved_deps))
        {
            trace::warning(_X("Dependency manifest %s does not contain an entry for %s"),
                resolved_deps.c_str(), _X("placeholder"));
        }

        // Check if the given version of the hostpolicy exists in servicing.
        if (hostpolicy_exists_in_svc(version, impl_dir))
        {
            return true;
        }

        // Get the expected directory that would contain hostpolicy.
        pal::string_t expected;
        if (is_framework_dependent)
        {
            // The hostpolicy is required to be in the root framework's location
            expected.assign(get_root_framework(fx_definitions).get_dir());
            assert(pal::directory_exists(expected));
        }
        else
        {
            // Native apps can be activated by muxer, native exe host or "corehost"
            // 1. When activated with dotnet.exe or corehost.exe, check for hostpolicy in the deps dir or
            //    app dir.
            // 2. When activated with native exe, the standalone host, check own directory.
            assert(mode != host_mode_t::invalid);
            expected = (mode == host_mode_t::apphost)
                ? dotnet_root
                : get_directory(specified_deps_file.empty() ? app_candidate : specified_deps_file);
        }

        // Check if hostpolicy exists in "expected" directory.
        trace::verbose(_X("The expected %s directory is [%s]"), LIBHOSTPOLICY_NAME, expected.c_str());
        if (library_exists_in_dir(expected, LIBHOSTPOLICY_NAME, nullptr))
        {
            impl_dir->assign(expected);
            return true;
        }


        return false;
    }

    int fx_muxer_t::read_config_and_execute(
        const arguments_t& arguments,
        const pal::string_t& host_command,
        const host_startup_info_t& host_info,
        const pal::string_t& app_candidate,
        const opt_map_t& opts,
        int new_argc,
        const pal::char_t** new_argv,
        host_mode_t mode,
        pal::char_t out_buffer[],
        int32_t buffer_size,
        int32_t* required_buffer_size)
    {
       // pal::string_t opts_fx_version = _X("--fx-version");
       // pal::string_t opts_roll_fwd_on_no_candidate_fx = _X("--roll-forward-on-no-candidate-fx");
       // pal::string_t opts_deps_file = _X("--depsfile");
       // pal::string_t opts_probe_path = _X("--additionalprobingpath");
       // pal::string_t opts_additional_deps = _X("--additional-deps");
       // pal::string_t opts_runtime_config = _X("--runtimeconfig");

        pal::string_t fx_version_specified;
        pal::string_t roll_fwd_on_no_candidate_fx;
        pal::string_t deps_file = _X("");//get_last_known_arg(opts, opts_deps_file, _X(""));
        pal::string_t additional_deps;
        pal::string_t runtime_config = _X("");// get_last_known_arg(opts, opts_runtime_config, _X(""));
        std::vector<pal::string_t> spec_probe_paths = std::vector<pal::string_t>();// opts.count(opts_probe_path) ? opts.find(opts_probe_path)->second : std::vector<pal::string_t>();

        if (!deps_file.empty() && !pal::realpath(&deps_file))
        {
            trace::error(_X("The specified deps.json [%s] does not exist"), deps_file.c_str());
            return StatusCode::InvalidArgFailure;
        }

        // Read config
        fx_definition_vector_t fx_definitions;
        auto app = new fx_definition_t();
        fx_definitions.push_back(std::unique_ptr<fx_definition_t>(app));

        int rc = read_config(*app, app_candidate, runtime_config);
        if (rc)
        {
            return rc;
        }

        auto app_config = app->get_runtime_config();
        bool is_framework_dependent = app_config.get_is_framework_dependent();

        // These settings are only valid for framework-dependent apps
        if (is_framework_dependent)
        {
            fx_version_specified = _X("");// get_last_known_arg(opts, opts_fx_version, _X(""));
            roll_fwd_on_no_candidate_fx = _X("");// get_last_known_arg(opts, opts_roll_fwd_on_no_candidate_fx, _X(""));
            additional_deps = _X("");// get_last_known_arg(opts, opts_additional_deps, _X(""));
        }

        // 'Roll forward on no candidate fx' is set to 1 (roll_fwd_on_no_candidate_fx_option::minor) by default. It can be changed through:
        // 1. Command line argument (--roll-forward-on-no-candidate-fx).
        // 2. Runtimeconfig json file ('rollForwardOnNoCandidateFx' property in "framework" section:).
        // 3. Runtimeconfig json file ('rollForwardOnNoCandidateFx' property), which is used as a default for lower level frameworks if they don't specify a value.
        // 4. DOTNET_ROLL_FORWARD_ON_NO_CANDIDATE_FX env var. Only defaults the app's config.
        // The conflicts will be resolved by following the priority rank described above (from 1 to 4).
        // The env var condition is verified in the config file processing
        if (!roll_fwd_on_no_candidate_fx.empty())
        {
            app_config.force_roll_fwd_on_no_candidate_fx(static_cast<roll_fwd_on_no_candidate_fx_option>(pal::xtoi(roll_fwd_on_no_candidate_fx.c_str())));
        }

        auto config = app_config;

        pal::string_t additional_deps_serialized;
        if (is_framework_dependent)
        {
            // Determine additional deps
            additional_deps_serialized = additional_deps;
            if (additional_deps_serialized.empty())
            {
                // additional_deps_serialized stays empty if DOTNET_ADDITIONAL_DEPS env var is not defined
                pal::getenv(_X("DOTNET_ADDITIONAL_DEPS"), &additional_deps_serialized);
            }

            // Obtain frameworks\platforms
            auto version = fx_version_specified;
            while (!config.get_fx_name().empty() && !config.get_fx_version().empty())
            {
                fx_definition_t* fx = resolve_fx(mode, config, host_info.dotnet_root, version);
                if (fx == nullptr)
                {
                    pal::string_t searched_version = fx_version_specified.empty() ? config.get_fx_version() : fx_version_specified;
                    handle_missing_framework_error(mode, config.get_fx_name(), searched_version, pal::string_t(), host_info.dotnet_root);
                    return FrameworkMissingFailure;
                }

                fx_definitions.push_back(std::unique_ptr<fx_definition_t>(fx));

                pal::string_t config_file;
                pal::string_t dev_config_file;
                get_runtime_config_paths(fx->get_dir(), config.get_fx_name(), &config_file, &dev_config_file);
                fx->parse_runtime_config(config_file, dev_config_file, &config, &app_config);

                config = fx->get_runtime_config();
                if (!config.is_valid())
                {
                    trace::error(_X("Invalid framework config.json [%s]"), config.get_path().c_str());
                    return StatusCode::InvalidConfigFile;
                }

                // Only the first framework can have a specified version (through --fx-version)
                version.clear();
            }
        }

        // Append specified probe paths first and then config file probe paths into realpaths.
        std::vector<pal::string_t> probe_realpaths;

        // The tfm is taken from the app.
        pal::string_t tfm = get_app(fx_definitions).get_runtime_config().get_tfm();

        for (const auto& path : spec_probe_paths)
        {
            append_probe_realpath(path, &probe_realpaths, tfm);
        }

        // Each framework can add probe paths
        for (const auto& fx : fx_definitions)
        {
            for (const auto& path : fx->get_runtime_config().get_probe_paths())
            {
                append_probe_realpath(path, &probe_realpaths, tfm);
            }
        }

        trace::verbose(_X("Executing as a %s app as per config file [%s]"),
            (is_framework_dependent ? _X("framework-dependent") : _X("self-contained")), config.get_path().c_str());

        pal::string_t impl_dir;
        if (!resolve_hostpolicy_dir(mode, host_info.dotnet_root, fx_definitions, app_candidate, deps_file, fx_version_specified, probe_realpaths, &impl_dir))
        {
            // set default core lib path
            return CoreHostLibMissingFailure;
        }

        corehost_init_t init(host_command, host_info, deps_file, additional_deps_serialized, probe_realpaths, mode, fx_definitions);
        auto initf = init.get_host_init_data();
        hostpolicy_init_t g_init;
        g_init = hostpolicy_init_t();

        if (!hostpolicy_init_t::init(&initf, &g_init))
        {
            return StatusCode::LibHostInitFailure;
        }

        deps_resolver_t resolver(g_init, arguments);

        pal::string_t resolver_errors;
        if (!resolver.valid(&resolver_errors))
        {
            trace::error(_X("Error initializing the dependency resolver: %s"), resolver_errors.c_str());
            return StatusCode::ResolverInitFailure;
        }
        // Setup breadcrumbs. Breadcrumbs are not enabled for API calls because they do not execute
        // the app and may be re-entry
        probe_paths_t probe_paths;
        std::unordered_set<pal::string_t> breadcrumbs;
        bool breadcrumbs_enabled = false;
        if (breadcrumbs_enabled)
        {
            pal::string_t policy_name = _STRINGIFY(HOST_POLICY_PKG_NAME);
            pal::string_t policy_version = _STRINGIFY(HOST_POLICY_PKG_VER);

            // Always insert the hostpolicy that the code is running on.
            breadcrumbs.insert(policy_name);
            breadcrumbs.insert(policy_name + _X(",") + policy_version);

            if (!resolver.resolve_probe_paths(&probe_paths, &breadcrumbs))
            {
                return StatusCode::ResolverResolveFailure;
            }
        }
        else
        {
            if (!resolver.resolve_probe_paths(&probe_paths, nullptr))
            {
                return StatusCode::ResolverResolveFailure;
            }
        }

        pal::string_t clr_path = probe_paths.coreclr;
        if (clr_path.empty() || !pal::realpath(&clr_path))
        {
            trace::error(_X("Could not resolve CoreCLR path. For more details, enable tracing by setting COREHOST_TRACE environment variable to 1"));;
            return StatusCode::CoreClrResolveFailure;
        }

        // Get path in which CoreCLR is present.
        pal::string_t clr_dir = get_directory(clr_path);

        // System.Private.CoreLib.dll is expected to be next to CoreCLR.dll - add its path to the TPA list.
        pal::string_t corelib_path = clr_dir;
        append_path(&corelib_path, CORELIB_NAME);

        // Append CoreLib path
        if (probe_paths.tpa.back() != PATH_SEPARATOR)
        {
            probe_paths.tpa.push_back(PATH_SEPARATOR);
        }

        probe_paths.tpa.append(corelib_path);
        probe_paths.tpa.push_back(PATH_SEPARATOR);

        pal::string_t clrjit_path = probe_paths.clrjit;
        if (clrjit_path.empty())
        {
            trace::warning(_X("Could not resolve CLRJit path"));
        }
        else if (pal::realpath(&clrjit_path))
        {
            trace::verbose(_X("The resolved JIT path is '%s'"), clrjit_path.c_str());
        }
        else
        {
            clrjit_path.clear();
            trace::warning(_X("Could not resolve symlink to CLRJit path '%s'"), probe_paths.clrjit.c_str());
        }

        // Build CoreCLR properties
        std::vector<const char*> property_keys = {
            "TRUSTED_PLATFORM_ASSEMBLIES",
            "NATIVE_DLL_SEARCH_DIRECTORIES",
            // "PLATFORM_RESOURCE_ROOTS",
             "AppDomainCompatSwitch",
             // Workaround: mscorlib does not resolve symlinks for AppContext.BaseDirectory dotnet/coreclr/issues/2128
             "APP_CONTEXT_BASE_DIRECTORY",
             "APP_CONTEXT_DEPS_FILES",
             "FX_DEPS_FILE",
             "PROBING_DIRECTORIES",
             "FX_PRODUCT_VERSION"
        };

        // Note: these variables' lifetime should be longer than coreclr_initialize.
        std::vector<char> tpa_paths_cstr, app_base_cstr, native_dirs_cstr, resources_dirs_cstr, fx_deps, deps, clrjit_path_cstr, probe_directories, clr_library_version;
        pal::pal_clrstring(probe_paths.tpa, &tpa_paths_cstr);
        pal::string_t clr_paths;
        clr_paths.append(arguments.app_root);

        //pal::pal_clrstring(args.app_root, &app_base_cstr);
        pal::pal_clrstring(clr_paths, &app_base_cstr);
        pal::pal_clrstring(probe_paths.native, &native_dirs_cstr);
        pal::pal_clrstring(probe_paths.resources, &resources_dirs_cstr);

        pal::string_t fx_deps_str;
        if (resolver.get_fx_definitions().size() >= 2)
        {
            // Use the root fx to define FX_DEPS_FILE
            fx_deps_str = get_root_framework(resolver.get_fx_definitions()).get_deps_file();
        }
        pal::pal_clrstring(fx_deps_str, &fx_deps);

        // Get all deps files
        pal::string_t allDeps;
        for (int i = 0; i < resolver.get_fx_definitions().size(); ++i)
        {
            allDeps += resolver.get_fx_definitions()[i]->get_deps_file();
            if (i < resolver.get_fx_definitions().size() - 1)
            {
                allDeps += _X(";");
            }
        }
        pal::pal_clrstring(allDeps, &deps);

        pal::pal_clrstring(resolver.get_lookup_probe_directories(), &probe_directories);

        if (resolver.is_framework_dependent())
        {
            pal::pal_clrstring(get_root_framework(resolver.get_fx_definitions()).get_found_version(), &clr_library_version);
        }
        else
        {
            pal::pal_clrstring(resolver.get_coreclr_library_version(), &clr_library_version);
        }

        std::string app_base_str(app_base_cstr.begin(), app_base_cstr.end());
        std::string fx_deps_str2(fx_deps.begin(), fx_deps.end());
        std::string probe_directories_str(probe_directories.begin(), probe_directories.end());
        std::string deps_str(deps.begin(), deps.end());
        std::string tpa_paths_str(tpa_paths_cstr.begin(), tpa_paths_cstr.end());
        std::string native_dirs_str(native_dirs_cstr.begin(), native_dirs_cstr.end());
        std::string resources_dirs_str(resources_dirs_cstr.begin(), resources_dirs_cstr.end());

        std::vector<const char*> property_values = {
            // TRUSTED_PLATFORM_ASSEMBLIES
            tpa_paths_cstr.data(),
            // NATIVE_DLL_SEARCH_DIRECTORIES
            native_dirs_cstr.data(),
            // PLATFORM_RESOURCE_ROOTS
           // resources_dirs_cstr.data(),
            // AppDomainCompatSwitch
            "UseLatestBehaviorWhenTFMNotSpecified",
            // APP_CONTEXT_BASE_DIRECTORY
            app_base_cstr.data(),
            // APP_CONTEXT_DEPS_FILES,
            deps.data(),
            // FX_DEPS_FILE
            fx_deps.data(),
            //PROBING_DIRECTORIES
            probe_directories.data(),
            //FX_PRODUCT_VERSION
            clr_library_version.data()
        };
        if (!clrjit_path.empty())
        {
            pal::pal_clrstring(clrjit_path, &clrjit_path_cstr);
            //property_keys.push_back("JIT_PATH");
           // property_values.push_back(clrjit_path_cstr.data());
        }

        bool set_app_paths = false;

        // Runtime options config properties.
        for (int i = 0; i < g_init.cfg_keys.size(); ++i)
        {
            // Provide opt-in compatible behavior by using the switch to set APP_PATHS
            if (pal::cstrcasecmp(g_init.cfg_keys[i].data(), "Microsoft.NETCore.DotNetHostPolicy.SetAppPaths") == 0)
            {
                set_app_paths = (pal::cstrcasecmp(g_init.cfg_values[i].data(), "true") == 0);
            }

            property_keys.push_back(g_init.cfg_keys[i].data());
            property_values.push_back(g_init.cfg_values[i].data());
        }

        size_t property_size = property_keys.size();
        assert(property_keys.size() == property_values.size());

        unsigned int exit_code = 1;

        // Check for host command(s)
        if (pal::strcasecmp(g_init.host_command.c_str(), _X("get-native-search-directories")) == 0)
        {
            // Verify property_keys[1] contains the correct information
            if (pal::cstrcasecmp(property_keys[1], "NATIVE_DLL_SEARCH_DIRECTORIES"))
            {
                trace::error(_X("get-native-search-directories failed to find NATIVE_DLL_SEARCH_DIRECTORIES property"));
                exit_code = HostApiFailed;
            }
            else
            {
                // assert(out_host_command_result != nullptr);
                //pal::clr_palstring(property_values[1], out_host_command_result);
                exit_code = 0; // Success
            }

            return exit_code;
        }

        // Bind CoreCLR
        trace::verbose(_X("CoreCLR path = '%s', CoreCLR dir = '%s'"), clr_path.c_str(), clr_dir.c_str());
        if (!coreclr::bind(clr_dir))
        {
            trace::error(_X("Failed to bind to CoreCLR at '%s'"), clr_path.c_str());
            return StatusCode::CoreClrBindFailure;
        }
        
        // Verbose logging
        if (trace::is_enabled())
        {
            for (size_t i = 0; i < property_size; ++i)
            {
                pal::string_t key, val;
                pal::clr_palstring(property_keys[i], &key);
                pal::clr_palstring(property_values[i], &val);
                trace::verbose(_X("Property %s = %s"), key.c_str(), val.c_str());
            }
        }

        std::vector<char> managed_application_path;
        pal::pal_clrstring(arguments.host_path, &managed_application_path);

        // Initialize CoreCLR
        coreclr::host_handle_t host_handle;
        coreclr::domain_id_t domain_id;
        auto hr = coreclr::initialize(
            managed_application_path.data(),
            "clrhost",
            property_keys.data(),
            property_values.data(),
            property_size,
            &host_handle,
            &domain_id);
        if (!SUCCEEDED(hr))
        {
            trace::error(_X("Failed to initialize CoreCLR, HRESULT: 0x%X"), hr);
            return StatusCode::CoreClrInitFailure;
        }

        // Initialize clr strings for arguments
        std::vector<std::vector<char>> argv_strs(arguments.app_argc);
        std::vector<const char*> argv(arguments.app_argc);
        for (int i = 0; i < arguments.app_argc; i++)
        {
            pal::pal_clrstring(arguments.app_argv[i], &argv_strs[i]);
            argv[i] = argv_strs[i].data();
        }

        if (trace::is_enabled())
        {
            pal::string_t arg_str;
            for (int i = 0; i < argv.size(); i++)
            {
                pal::string_t cur;
                pal::clr_palstring(argv[i], &cur);
                arg_str.append(cur);
                arg_str.append(_X(","));
            }
            trace::info(_X("Launch host: %s, app: %s, argc: %d, args: %s"), arguments.host_path.c_str(),
                arguments.managed_application.c_str(), arguments.app_argc, arg_str.c_str());
        }
        typedef int (STDMETHODCALLTYPE MainMethodFp)();
        MainMethodFp *pfnDelegate = NULL;
        hr = coreclr::create_delegate(
            host_handle,
            domain_id,
            arguments.assembly_name,
            arguments.type_name,
            arguments.method_name,
            reinterpret_cast<VOID**>(&pfnDelegate));
        if (!SUCCEEDED(hr))
        {
            trace::error(_X("Failed to create delegate for managed library, HRESULT: 0x%X"), hr);
            return StatusCode::CoreClrExeFailure;
        }

        // Execute the .NET native delegate
        pfnDelegate();

        hr = coreclr::shutdown(host_handle, domain_id, (int*)&exit_code);
        if (!SUCCEEDED(hr))
        {
            trace::warning(_X("Failed to shut down CoreCLR, HRESULT: 0x%X"), hr);
        }

        coreclr::unload();

        return exit_code;
    }
}