#pragma once
#ifndef CORELOAD_DLL_H
#define CORELOAD_DLL_H

#include "targetver.h"
#include "deps_resolver.h"
#include "corehost.h"
#include "status_code.h"

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
struct core_host_arguments
{
    unsigned char verbose;
    unsigned char reserved[7];
    pal::char_t   assembly_file_path[MAX_PATH];
    pal::char_t   core_root_path[MAX_PATH];
    pal::char_t   core_libraries_path[MAX_PATH];
};

// Arguments for executing a function located in a .NET assembly,
// with optional arguments passed to the function call
struct assembly_function_call
{
    pal::char_t   assembly_name[FunctionNameSize];
    pal::char_t   class_name[FunctionNameSize];
    pal::char_t   function_name[FunctionNameSize];
    unsigned char arguments[AssemblyFunCallArgsSize];
};

struct core_load_arguments
{
    const unsigned char* user_data;
    unsigned long        user_data_size;
};

struct remote_entry_info
{
    unsigned long       host_process_id;
    core_load_arguments arguments;
};

// DLL exports used for starting, executing in, and stopping the .NET Core runtime

// Create a native function delegate for a function inside a .NET assembly
DllApi
int
CreateAssemblyDelegate(
    const char* assembly_name,
    const char* type_name,
    const char* method_name,
    void**      pfnDelegate
);

// Execute a function located in a .NET assembly by creating a native delegate
DllApi
int
ExecuteAssemblyFunction(
    const assembly_function_call* arguments
);

// Host the .NET Core runtime in the current application
DllApi
int
StartCoreCLR(
    const core_host_arguments* arguments
);

// Stop the .NET Core host in the current application
DllApi
int
UnloadRuntime();
#endif // CORELOAD_DLL_H