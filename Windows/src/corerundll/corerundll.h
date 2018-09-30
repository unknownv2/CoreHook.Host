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
#define AssemblyFunCallArgsSize        512

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
DllApi
VOID
UnloadRunTime(
    VOID
);

DllApi
VOID
ExecuteAssemblyFunction(
    IN CONST AssemblyFunctionCall* args
);

DllApi
VOID
LoadAssembly(
    IN CONST BinaryLoaderArgs* args
);

DllApi
VOID
ExecuteAssembly(
    IN CONST BinaryLoaderArgs* args
);

DllApi
DWORD
StartCLRAndLoadAssembly(
    IN CONST WCHAR*  dllPath,
    IN CONST BOOLEAN verbose,
    IN CONST BOOLEAN waitForDebugger,
    IN CONST WCHAR*  coreRoot,
    IN CONST WCHAR*  coreLibraries,
    IN CONST BOOLEAN executeAssembly
);
