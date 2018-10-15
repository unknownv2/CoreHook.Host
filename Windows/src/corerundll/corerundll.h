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
    BOOLEAN    Reserved[6];
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

// DLL exports used for starting, executing in, and stopping the .NET Core runtime


// Host the .NET Core runtime in the current application
DllApi
HRESULT
StartCoreCLR(
    IN CONST BinaryLoaderArgs *args
);

// Create a native function delegate for a function inside a .NET assembly
DllApi
HRESULT
CreateAssemblyDelegate(
    IN CONST WCHAR  *assembly,
    IN CONST WCHAR  *type,
    IN CONST WCHAR  *entry,
    _Inout_  PVOID  *pfnDelegate
);

// Execute a function located in a .NET assembly
DllApi
VOID
ExecuteAssemblyFunction(
    IN CONST AssemblyFunctionCall *args
);

// Stop the .NET Core host in the current application
DllApi
VOID
UnloadRunTime(
    VOID
);