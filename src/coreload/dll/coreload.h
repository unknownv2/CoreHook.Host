#pragma once
#ifndef CORELOAD_DLL_H
#define CORELOAD_DLL_H

#include "targetver.h"
#include "deps_resolver.h"
#include "corehost.h"
#include "status_code.h"

// The max length of a function to be executed in a .NET class
#define max_function_name_size                  256

// The max length of arguments to be parsed and passed to a .NET function
#define assembly_function_arguments_size        12

using namespace coreload;

// Arguments for hosting the .NET Core runtime and loading an assembly into the
struct core_host_arguments
{
    unsigned char verbose;
    unsigned char reserved[7];
    pal::char_t   assembly_file_path[MAX_PATH];
    pal::char_t   core_root_path[MAX_PATH];
};

// Arguments for executing a function located in a .NET assembly,
// with optional arguments passed to the function call
struct assembly_function_call
{
    pal::char_t   assembly_name[max_function_name_size];
    pal::char_t   class_name[max_function_name_size];
    pal::char_t   function_name[max_function_name_size];
    unsigned char arguments[assembly_function_arguments_size];
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
SHARED_API int CreateAssemblyDelegate(
    const char* assembly_name,
    const char* type_name,
    const char* method_name,
    void**      pfnDelegate
);

// Execute a function located in a .NET assembly by creating a native delegate
SHARED_API int ExecuteAssemblyFunction(
    const assembly_function_call* arguments
);

// Host the .NET Core runtime in the current application
SHARED_API int StartCoreCLR(
    const core_host_arguments* arguments
);

// Stop the .NET Core host in the current application
SHARED_API int UnloadRuntime();

#endif // CORELOAD_DLL_H