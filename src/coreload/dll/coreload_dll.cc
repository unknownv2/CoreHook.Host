#include "coreload_dll.h"

// Start the .NET Core runtime in the current application
int
StartCoreCLRInternal(
    const pal::char_t   *dllPath,
    const unsigned char verbose,
    const pal::char_t   *coreRoot,
    const pal::char_t   *coreLibraries) {
    if (verbose) {
        trace::enable();
    }
    host_startup_info_t startup_info;
    arguments_t arguments;
    // Used to the find dotnet dependencies
    startup_info.dotnet_root = coreRoot;

    arguments.managed_application = dllPath;
    arguments.app_root = get_directory(arguments.managed_application);

    return fx_muxer_t::initialize_clr(
        arguments,
        startup_info,
        host_mode_t::muxer);
}

// Host the .NET Core runtime in the current application
DllApi
int
StartCoreCLR(const CoreLoadArgs *args) {
    return StartCoreCLRInternal(args->BinaryFilePath, args->Verbose, args->CoreRootPath, args->CoreLibrariesPath);
}

// Create a native function delegate for a function inside a .NET assembly
DllApi
int
CreateAssemblyDelegate(
    const char *assembly_name,
    const char *type_name,
    const char *method_name,
    void **pfnDelegate) {
    return fx_muxer_t::create_delegate(
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
    typedef void (STDMETHODCALLTYPE MainMethodFp)(const VOID* args);
    MainMethodFp *pfnDelegate = nullptr;

    if (SUCCEEDED((exit_code = CreateAssemblyDelegate(assembly, type, entry, reinterpret_cast<PVOID*>(&pfnDelegate))))) {
        RemoteEntryInfo entryInfo = { 0 };
        entryInfo.HostPID = GetCurrentProcessId();

        const auto remoteArgs = reinterpret_cast<const RemoteFunctionArgs*>(arguments);
        if (remoteArgs != nullptr) {
            // construct a hex string for the address of the entryInfo parameter
            // which is passed to the .NET delegate function and execute the delegate
            entryInfo.Args.UserData = remoteArgs->UserData;
            entryInfo.Args.UserDataSize = remoteArgs->UserDataSize;

            pfnDelegate(&entryInfo);
        }
        else {
            // No arguments were supplied to pass to the delegate function so pass an
            // empty string
            pfnDelegate(nullptr);
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
UnloadRuntime(void) {
    return fx_muxer_t::unload_runtime();
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved) {
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
