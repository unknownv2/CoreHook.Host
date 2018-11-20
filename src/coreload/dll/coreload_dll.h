#pragma once
#ifndef CORELOAD_DLL_H
#define CORELOAD_DLL_H

#include "targetver.h"
#include "coreload.h"

// Function export macro
#ifdef _USRDLL  
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

using namespace coreload;

// Arguments for hosting the .NET Core runtime and loading an assembly into the
struct CoreLoadArgs
{
    unsigned char    Verbose;
    unsigned char    Reserved[7];
    pal::char_t      BinaryFilePath[MAX_PATH];
    pal::char_t      CoreRootPath[MAX_PATH];
    pal::char_t      CoreLibrariesPath[MAX_PATH];
};

// Arguments for executing a function located in a .NET assembly,
// with optional arguments passed to the function call
struct AssemblyFunctionCall
{
    char           Assembly[FunctionNameSize];
    char           Class[FunctionNameSize];
    char           Function[FunctionNameSize];
    unsigned char  Arguments[AssemblyFunCallArgsSize];
};

struct RemoteFunctionArgs
{
    const unsigned char *UserData;
    unsigned long       UserDataSize;
};

struct RemoteEntryInfo
{
    unsigned long      HostPID;
    RemoteFunctionArgs Args;
};

// DLL exports used for starting, executing in, and stopping the .NET Core runtime

// Create a native function delegate for a function inside a .NET assembly
DllApi
int
CreateAssemblyDelegate(
    const char *assembly_name,
    const char *type_name,
    const char *method_name,
    void       **pfnDelegate
);

// Execute a function located in a .NET assembly by creating a native delegate
DllApi
int
ExecuteAssemblyFunction(
    const AssemblyFunctionCall *args
);

// Host the .NET Core runtime in the current application
DllApi
int
StartCoreCLR(
    const CoreLoadArgs *args
);

// Stop the .NET Core host in the current application
DllApi
int
UnloadRuntime();
#endif // CORELOAD_DLL_H