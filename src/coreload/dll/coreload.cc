#include "coreload.h"

int ValidateArgument(
    const pal::char_t* argument,
    size_t max_size)
{
    if (argument != nullptr)
    {
        const size_t string_length = pal::strnlen(argument, max_size);
        if (string_length == 0 || string_length >= max_size)
        {
            return StatusCode::InvalidArgFailure;
        }
    }
    else
    {
        return StatusCode::InvalidArgFailure;
    }
    return StatusCode::Success;
}

bool inline IsValidCoreHostArgument(
    const pal::char_t* argument,
    size_t max_size)
{
    return ValidateArgument(argument, max_size) == StatusCode::Success;
}

// Start the .NET Core runtime in the current application
int StartCoreCLRInternal(
    const pal::char_t*  assembly_path,
    const pal::char_t*  core_root,
    const unsigned char verbose_log)
{
    if (verbose_log)
    {
        trace::enable();
    }
    host_startup_info_t startup_info;
    arguments_t arguments;
  
    startup_info.dotnet_root = core_root;

    arguments.managed_application = assembly_path;
    arguments.app_root = get_directory(arguments.managed_application);

    return corehost::initialize_clr(
        arguments,
        startup_info,
        host_mode_t::muxer);
}

// Host the .NET Core runtime in the current application
SHARED_API int StartCoreCLR(
    const core_host_arguments* arguments)
{
    if (arguments == nullptr 
        || !IsValidCoreHostArgument(arguments->assembly_file_path, MAX_PATH)
        || !IsValidCoreHostArgument(arguments->core_root_path, MAX_PATH))
    {
        return StatusCode::InvalidArgFailure;
    }

    return StartCoreCLRInternal(arguments->assembly_file_path, arguments->core_root_path, arguments->verbose);
}

// Create a native function delegate for a function inside a .NET assembly
SHARED_API int CreateAssemblyDelegate(
    const char* assembly_name,
    const char* type_name,
    const char* method_name,
    void**      pfnDelegate)
{
    return corehost::create_delegate(
        assembly_name,
        type_name,
        method_name,
        pfnDelegate
    );
}

// Execute a method from a class located inside a .NET assembly
int ExecuteAssemblyClassFunction(
    const char* assembly,
    const char* type,
    const char* entry,
    const unsigned char* arguments)
{
    int exit_code = StatusCode::HostApiFailed;
    typedef void (STDMETHODCALLTYPE load_plugin_fn)(const void *load_plugin_arguments);
    load_plugin_fn* load_plugin_delegate = nullptr;

    if (SUCCEEDED(exit_code = CreateAssemblyDelegate(assembly, type, entry, reinterpret_cast<PVOID*>(&load_plugin_delegate))))
    {
        remote_entry_info entry_info = { 0 };
        entry_info.host_process_id = GetCurrentProcessId();

        const auto remote_arguments = reinterpret_cast<const core_load_arguments*>(arguments);
        if (remote_arguments != nullptr)
        {
            // Construct and pass the remote user parameters to the .NET delegate
            entry_info.arguments.user_data_size = remote_arguments->user_data_size;
            entry_info.arguments.user_data = remote_arguments->user_data_size ? remote_arguments->user_data : nullptr;

            load_plugin_delegate(&entry_info);
        }
        else
        {
            // No arguments were supplied to pass to the delegate function
            load_plugin_delegate(nullptr);
        }
    }
    else
    {
        exit_code = StatusCode::InvalidArgFailure;
    }
    return exit_code;
}

// Execute a function located in a .NET assembly by creating a native delegate
SHARED_API int ExecuteAssemblyFunction(const assembly_function_call* arguments) 
{
    if (arguments == nullptr 
        || !IsValidCoreHostArgument(arguments->assembly_name, max_function_name_size)
        || !IsValidCoreHostArgument(arguments->class_name, max_function_name_size)
        || !IsValidCoreHostArgument(arguments->function_name, max_function_name_size))
    {
        return StatusCode::InvalidArgFailure;
    }

    std::vector<char> assembly_name, class_name, function_name;
    pal::pal_clrstring(arguments->assembly_name, &assembly_name);
    pal::pal_clrstring(arguments->class_name, &class_name);
    pal::pal_clrstring(arguments->function_name, &function_name);

    return ExecuteAssemblyClassFunction(assembly_name.data(), class_name.data(), function_name.data(), arguments->arguments);
}

// Shutdown the .NET Core runtime
SHARED_API int UnloadRuntime()
{
    return corehost::unload_runtime();
}

BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD   ul_reason_for_call,
    LPVOID  lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
        default:
            break;
    }
    return TRUE;
}
