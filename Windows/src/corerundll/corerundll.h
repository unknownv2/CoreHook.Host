#pragma once
#include <windows.h>

// Function export macro
#ifdef _EXPORTING  
#define DllExport    __declspec(dllexport)
#else
#define DllExport    __declspec(dllimport)
#endif

// Export functions with their plain name
#define DllApi extern "C" DllExport

// The max length of a function to be executed in a .NET class
#define FunctionNameSize               256

// The max length of arguments to be parsed and passed to a .NET function
#define AssemblyFunCallArgsSize        12

// Arguments for hosting the .NET Core runtime and loading an assembly into the
struct BinaryLoaderArgs
{
    BOOLEAN    Verbose;
    BOOLEAN    WaitForDebugger;
    BOOLEAN    StartAssembly;
    BOOLEAN    Reserved[5];
    WCHAR      BinaryFilePath[MAX_PATH];
    WCHAR      CoreRootPath[MAX_PATH];
    WCHAR      CoreLibrariesPath[MAX_PATH];
};

// Arguments for executing a function located in a .NET assembly,
// with optional arguments passed to the function call
struct AssemblyFunctionCall
{
    WCHAR   Assembly[FunctionNameSize];
    WCHAR   Class[FunctionNameSize];
    WCHAR   Function[FunctionNameSize];
    BYTE    Arguments[AssemblyFunCallArgsSize];
};

struct RemoteFunctionArgs
{
    CONST BYTE *UserData;
    ULONG       UserDataSize;
};

struct RemoteEntryInfo
{
    ULONG              HostPID;
    RemoteFunctionArgs Args;
};

// DLL exports used for starting, executing in, and stopping the CoreCLR

// Stop the .NET Core host in the current application
DllApi
VOID
UnloadRunTime(
    VOID
);

// Execute a function in a .NET assembly that has been loaded in the .NET Core runtime
DllApi
VOID
ExecuteAssemblyFunction(
    IN CONST AssemblyFunctionCall *args
);

// Load a .NET assembly into the .NET Core runtime
DllApi
VOID
LoadAssembly(
    IN CONST BinaryLoaderArgs *args
);

// Host the CoreCLR in the current application and load a .NET appliaction into the runtime
// and execute its entry point
DllApi
VOID
ExecuteAssembly(
    IN CONST BinaryLoaderArgs *args
);

// Host the CoreCLR in the current application and load a .NET assembly into the runtime
// which can be executed immediately using its entry point
DllApi
DWORD
StartCLRAndLoadAssembly(
    IN CONST WCHAR   *dllPath,
    IN CONST BOOLEAN verbose,
    IN CONST BOOLEAN waitForDebugger,
    IN CONST WCHAR   *coreRoot,
    IN CONST WCHAR   *coreLibraries,
    IN CONST BOOLEAN executeAssembly
);
