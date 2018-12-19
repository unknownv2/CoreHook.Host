#include "coreload.h"

// Start the .NET Core runtime in the current application
int
StartCoreCLRInternal(
    const pal::char_t*  assembly_path,
    const pal::char_t*  core_root,
    const unsigned char verbose_log) {
    if (verbose_log) {
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
DllApi
int
StartCoreCLR(const core_host_arguments* arguments) {
    return StartCoreCLRInternal(arguments->assembly_file_path, arguments->core_root_path, arguments->verbose);
}

// Create a native function delegate for a function inside a .NET assembly
DllApi
int
CreateAssemblyDelegate(
    const char* assembly_name,
    const char* type_name,
    const char* method_name,
    void**      pfnDelegate) {
    return corehost::create_delegate(
        assembly_name,
        type_name,
        method_name,
        pfnDelegate
    );
}

// Execute a method from a class located inside a .NET assembly
int
ExecuteAssemblyClassFunction(
    const char* assembly,
    const char* type,
    const char* entry,
    const unsigned char* arguments) {
    int exit_code = StatusCode::HostApiFailed;
    typedef void (STDMETHODCALLTYPE load_plugin_fn)(const void *load_plugin_arguments);
    load_plugin_fn* load_plugin_delegate = nullptr;

    if (SUCCEEDED(exit_code = CreateAssemblyDelegate(assembly, type, entry, reinterpret_cast<PVOID*>(&load_plugin_delegate)))) {
        remote_entry_info entryInfo = { 0 };
        entryInfo.host_process_id = GetCurrentProcessId();

        const auto remote_arguments = reinterpret_cast<const core_load_arguments*>(arguments);
        if (remote_arguments != nullptr) {
            // Construct and pass the remote user parameters to the .NET delegate
            entryInfo.arguments.user_data_size = remote_arguments->user_data_size;
            entryInfo.arguments.user_data = remote_arguments->user_data_size ? remote_arguments->user_data : nullptr;

            load_plugin_delegate(&entryInfo);
        }
        else {
            // No arguments were supplied to pass to the delegate function
            load_plugin_delegate(nullptr);
        }
    }
    else {
        exit_code = StatusCode::InvalidArgFailure;
    }
    return exit_code;
}

// Execute a function located in a .NET assembly by creating a native delegate
DllApi
int
ExecuteAssemblyFunction(const assembly_function_call* arguments) {
    std::vector<char> assemblyName, className, functionName;
    pal::pal_clrstring(arguments->assembly_name, &assemblyName);
    pal::pal_clrstring(arguments->class_name, &className);
    pal::pal_clrstring(arguments->function_name, &functionName);

    return ExecuteAssemblyClassFunction(assemblyName.data(), className.data(), functionName.data(), arguments->arguments);
}

// Shutdown the .NET Core runtime
DllApi
int
UnloadRuntime() {
    return corehost::unload_runtime();
}

BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD   ul_reason_for_call,
    LPVOID  lpReserved) {
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
