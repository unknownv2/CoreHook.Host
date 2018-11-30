#include "coreload.h"

// Start the .NET Core runtime in the current application
int
StartCoreCLRInternal(
    const pal::char_t   *dllPath,
    const unsigned char verbose,
    const pal::char_t   *coreRoot) {
    if (verbose) {
        trace::enable();
    }
    host_startup_info_t startup_info;
    arguments_t arguments;
  
    startup_info.dotnet_root = coreRoot;

    arguments.managed_application = dllPath;
    arguments.app_root = get_directory(arguments.managed_application);

    return corehost::initialize_clr(
        arguments,
        startup_info,
        host_mode_t::muxer);
}

// Host the .NET Core runtime in the current application
DllApi
int
StartCoreCLR(const CoreLoadArgs *args) {
    return StartCoreCLRInternal(args->BinaryFilePath, args->Verbose, args->CoreRootPath);
}

// Create a native function delegate for a function inside a .NET assembly
DllApi
int
CreateAssemblyDelegate(
    const char *assembly_name,
    const char *type_name,
    const char *method_name,
    void **pfnDelegate) {
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
    const char *assembly,
    const char *type,
    const char *entry,
    const unsigned char *arguments) {
    int exit_code = StatusCode::HostApiFailed;
    typedef void (STDMETHODCALLTYPE LoadMethodFp)(const void *args);
    LoadMethodFp *pfnLoadDelegate = nullptr;

    if (SUCCEEDED((exit_code = CreateAssemblyDelegate(assembly, type, entry, reinterpret_cast<PVOID*>(&pfnLoadDelegate))))) {
        RemoteEntryInfo entryInfo = { 0 };
        entryInfo.HostPID = GetCurrentProcessId();

        const auto remoteArgs = reinterpret_cast<const RemoteFunctionArgs*>(arguments);
        if (remoteArgs != nullptr) {
            // Construct and pass the remote user parameters to the .NET delegate
            entryInfo.Args.UserDataSize = remoteArgs->UserDataSize;
            entryInfo.Args.UserData = remoteArgs->UserDataSize ? remoteArgs->UserData : nullptr;

            pfnLoadDelegate(&entryInfo);
        }
        else {
            // No arguments were supplied to pass to the delegate function
            pfnLoadDelegate(nullptr);
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
ExecuteAssemblyFunction(const AssemblyFunctionCall *args) {
    return ExecuteAssemblyClassFunction(args->Assembly, args->Class, args->Function, args->Arguments);
}

// Shutdown the .NET Core runtime
DllApi
int
UnloadRuntime() {
    return corehost::unload_runtime();
}

BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved) {
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
