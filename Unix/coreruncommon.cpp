#include <cstdlib>
#include <cstring>
#include <assert.h>
#include <dirent.h>
#include <dlfcn.h>
#include <limits.h>
#include <set>
#include <string>
#include <string.h>
#include <sys/stat.h>
#if defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/param.h>
#endif
#if defined(HAVE_SYS_SYSCTL_H) || defined(__FreeBSD__)
#include <sys/sysctl.h>
#endif
#include "coreruncommon.h"
#include "coreclrhost.h"
#include <unistd.h>
#ifndef SUCCEEDED
#define SUCCEEDED(Status) ((Status) >= 0)
#endif // !SUCCEEDED

#include <csignal>
#include <pthread.h>

// Name of the environment variable controlling server GC.
// If set to 1, server GC is enabled on startup. If 0, server GC is
// disabled. Server GC is off by default.
static const char* serverGcVar = "CORECLR_SERVER_GC";

// Name of environment variable to control "System.Globalization.Invariant"
// Set to 1 for Globalization Invariant mode to be true. Default is false.
static const char* globalizationInvariantVar = "CORECLR_GLOBAL_INVARIANT";

#if defined(__linux__)
#define symlinkEntrypointExecutable "/proc/self/exe"
#elif !defined(__APPLE__)
#define symlinkEntrypointExecutable "/proc/curproc/exe"
#endif

typedef void* (*thread_start_routine)(void*);

#define BITS_PER_BYTE (8)

#define LOCAL_PARAM 1
/* extract the n-th bit of x */
#define GET_BIT(x, n) ((((x)[(n) / BITS_PER_BYTE]) & (0x1 << ((n) % BITS_PER_BYTE))) != 0)

struct RemoteThreadArgs {
    int32_t    Status;
    // placeholder for custom flags
    uint32_t    ProcFlags;
    // store result of call
    uint64_t    Result;
    uint32_t    ThreadAttributes;
    uint32_t    CreationFlags;
    size_t      StackSize;
    void *      StartAddress;
    void *      Params;
};

struct BinaryLoaderArgs
{
	bool Verbose;
	bool WaitForDebugger;
	bool StartAssembly;
	char Reserved[5];
	char BinaryFilePath[PATH_MAX];
	char CoreRootPath[PATH_MAX];
	char CoreLibrariesPath[PATH_MAX];
};

#if defined (__linux__)
RemoteThreadArgs * m_RemoteThread = NULL;

EXPORT void* RemoteThreadMailbox = NULL;

void * CreateRemoteThread(void * args)
{
    m_RemoteThread = (RemoteThreadArgs*)RemoteThreadMailbox;    
    if(m_RemoteThread == NULL) {
        printf("RemoteThreading not initialized!\n");
        return NULL;
    }

    while(1) {
        sleep(1);

        if(m_RemoteThread->Status == 1)
        {
            printf("Received function call %p\n", m_RemoteThread->StartAddress);

            if(m_RemoteThread->StartAddress != NULL) {
                thread_start_routine func = (thread_start_routine)m_RemoteThread->StartAddress;
                void * ret = func(m_RemoteThread->Params);
                // clear args buffer
                memset(m_RemoteThread, 0, sizeof(RemoteThreadArgs));
                // set function result
                m_RemoteThread->Result = (uint64_t)ret;
            }
        }
    }
    return NULL;
}
#endif
extern "C"
bool GetEntrypointExecutableAbsolutePath(std::string& entrypointExecutable)
{
    bool result = false;

    entrypointExecutable.clear();

    // Get path to the executable for the current process using
    // platform specific means.
#if defined(__APPLE__)

    // On Mac, we ask the OS for the absolute path to the entrypoint executable
    uint32_t lenActualPath = 0;
    if (_NSGetExecutablePath(nullptr, &lenActualPath) == -1)
    {
        // OSX has placed the actual path length in lenActualPath,
        // so re-attempt the operation
        std::string resizedPath(lenActualPath, '\0');
        char *pResizedPath = const_cast<char *>(resizedPath.c_str());
        if (_NSGetExecutablePath(pResizedPath, &lenActualPath) == 0)
        {
            entrypointExecutable.assign(pResizedPath);
            result = true;
        }
    }
#elif defined (__FreeBSD__)
    static const int name[] = {
        CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1
    };
    char path[PATH_MAX];
    size_t len;

    len = sizeof(path);
    if (sysctl(name, 4, path, &len, nullptr, 0) == 0)
    {
        entrypointExecutable.assign(path);
        result = true;
    }
    else
    {
        // ENOMEM
        result = false;
    }
#elif defined(__NetBSD__) && defined(KERN_PROC_PATHNAME)
    static const int name[] = {
        CTL_KERN, KERN_PROC_ARGS, -1, KERN_PROC_PATHNAME,
    };
    char path[MAXPATHLEN];
    size_t len;

    len = sizeof(path);
    if (sysctl(name, __arraycount(name), path, &len, NULL, 0) != -1)
    {
        entrypointExecutable.assign(path);
        result = true;
    }
    else
    {
        result = false;
    }
#else
    // On other OSs, return the symlink that will be resolved by GetAbsolutePath
    // to fetch the entrypoint EXE absolute path, inclusive of filename.
    result = GetAbsolutePath(symlinkEntrypointExecutable, entrypointExecutable);
#endif

    return result;
}

extern "C"
bool GetAbsolutePath(const char* path, std::string& absolutePath)
{
    bool result = false;

    char realPath[PATH_MAX];
    if (realpath(path, realPath) != nullptr && realPath[0] != '\0')
    {
        absolutePath.assign(realPath);
        // realpath should return canonicalized path without the trailing slash
        assert(absolutePath.back() != '/');

        result = true;
    }

    return result;
}
extern "C"
bool GetDirectory(const char* absolutePath, std::string& directory)
{
    directory.assign(absolutePath);
    size_t lastSlash = directory.rfind('/');
    if (lastSlash != std::string::npos)
    {
        directory.erase(lastSlash);
        return true;
    }

    return false;
}

extern "C"
bool GetClrFilesAbsolutePath(const char* currentExePath, const char* clrFilesPath, std::string& clrFilesAbsolutePath)
{
    std::string clrFilesRelativePath;
    const char* clrFilesPathLocal = clrFilesPath;
    if (clrFilesPathLocal == nullptr)
    {
        // There was no CLR files path specified, use the folder of the corerun/coreconsole
        if (!GetDirectory(currentExePath, clrFilesRelativePath))
        {
            perror("Failed to get directory from argv[0]");
            return false;
        }

        clrFilesPathLocal = clrFilesRelativePath.c_str();

        // TODO: consider using an env variable (if defined) as a fall-back.
        // The windows version of the corerun uses core_root env variable
    }

    if (!GetAbsolutePath(clrFilesPathLocal, clrFilesAbsolutePath))
    {
        perror("Failed to convert CLR files path to absolute path");
        return false;
    }

    return true;
}

extern "C"
void AddFilesFromDirectoryToTpaList(const char* directory, std::string& tpaList)
{
    const char * const tpaExtensions[] = {
                ".ni.dll",      // Probe for .ni.dll first so that it's preferred if ni and il coexist in the same dir
                ".dll",
                ".ni.exe",
                ".exe",
                };

    DIR* dir = opendir(directory);
    if (dir == nullptr)
    {
        return;
    }

    std::set<std::string> addedAssemblies;

    // Walk the directory for each extension separately so that we first get files with .ni.dll extension,
    // then files with .dll extension, etc.
    for (int extIndex = 0; extIndex < sizeof(tpaExtensions) / sizeof(tpaExtensions[0]); extIndex++)
    {
        const char* ext = tpaExtensions[extIndex];
        int extLength = strlen(ext);

        struct dirent* entry;

        // For all entries in the directory
        while ((entry = readdir(dir)) != nullptr)
        {
            // We are interested in files only
            switch (entry->d_type)
            {
            case DT_REG:
                break;

            // Handle symlinks and file systems that do not support d_type
            case DT_LNK:
            case DT_UNKNOWN:
                {
                    std::string fullFilename;

                    fullFilename.append(directory);
                    fullFilename.append("/");
                    fullFilename.append(entry->d_name);

                    struct stat sb;
                    if (stat(fullFilename.c_str(), &sb) == -1)
                    {
                        continue;
                    }

                    if (!S_ISREG(sb.st_mode))
                    {
                        continue;
                    }
                }
                break;

            default:
                continue;
            }

            std::string filename(entry->d_name);

            // Check if the extension matches the one we are looking for
            int extPos = filename.length() - extLength;
            if ((extPos <= 0) || (filename.compare(extPos, extLength, ext) != 0))
            {
                continue;
            }

            std::string filenameWithoutExt(filename.substr(0, extPos));

            // Make sure if we have an assembly with multiple extensions present,
            // we insert only one version of it.
            if (addedAssemblies.find(filenameWithoutExt) == addedAssemblies.end())
            {
                addedAssemblies.insert(filenameWithoutExt);

                tpaList.append(directory);
                tpaList.append("/");
                tpaList.append(filename);
                tpaList.append(":");
            }
        }

        // Rewind the directory stream to be able to iterate over it for the next extension
        rewinddir(dir);
    }

    closedir(dir);
}

const char* GetEnvValueBoolean(const char* envVariable)
{
    const char* envValue = std::getenv(envVariable);
    if (envValue == nullptr)
    {
        envValue = "0";
    }
    // CoreCLR expects strings "true" and "false" instead of "1" and "0".
    return (std::strcmp(envValue, "1") == 0 || strcasecmp(envValue, "true") == 0) ? "true" : "false";
}

int ExecuteManagedAssembly(
            const char* currentExeAbsolutePath,
            const char* clrFilesAbsolutePath,
            const char* managedAssemblyAbsolutePath,
            int managedAssemblyArgc,
            const char** managedAssemblyArgv)
{
    // Indicates failure
    int exitCode = -1;
#ifdef _ARM_
    // libunwind library is used to unwind stack frame, but libunwind for ARM
    // does not support ARM vfpv3/NEON registers in DWARF format correctly.
    // Therefore let's disable stack unwinding using DWARF information
    // See https://github.com/dotnet/coreclr/issues/6698
    //
    // libunwind use following methods to unwind stack frame.
    // UNW_ARM_METHOD_ALL          0xFF
    // UNW_ARM_METHOD_DWARF        0x01
    // UNW_ARM_METHOD_FRAME        0x02
    // UNW_ARM_METHOD_EXIDX        0x04
    putenv(const_cast<char *>("UNW_ARM_UNWIND_METHOD=6"));
#endif // _ARM_

    std::string coreClrDllPath(clrFilesAbsolutePath);
    coreClrDllPath.append("/");
    coreClrDllPath.append(coreClrDll);

    if (coreClrDllPath.length() >= PATH_MAX)
    {
        fprintf(stderr, "Absolute path to libcoreclr.so too long\n");
        return -1;
    }

    // Get just the path component of the managed assembly path
    std::string appPath;
    GetDirectory(managedAssemblyAbsolutePath, appPath);

    std::string tpaList;
    if (strlen(managedAssemblyAbsolutePath) > 0)
    {
        // Target assembly should be added to the tpa list. Otherwise corerun.exe
        // may find wrong assembly to execute.
        // Details can be found at https://github.com/dotnet/coreclr/issues/5631
        tpaList = managedAssemblyAbsolutePath;
        tpaList.append(":");
    }

    // Construct native search directory paths
    std::string nativeDllSearchDirs(appPath);
    char *coreLibraries = getenv("CORE_LIBRARIES");
    if (coreLibraries)
    {
        nativeDllSearchDirs.append(":");
        nativeDllSearchDirs.append(coreLibraries);
        if (std::strcmp(coreLibraries, clrFilesAbsolutePath) != 0)
        {
            AddFilesFromDirectoryToTpaList(coreLibraries, tpaList);
        }
    }

    nativeDllSearchDirs.append(":");
    nativeDllSearchDirs.append(clrFilesAbsolutePath);

    AddFilesFromDirectoryToTpaList(clrFilesAbsolutePath, tpaList);

    void* coreclrLib = dlopen(coreClrDllPath.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (coreclrLib != nullptr)
    {
        coreclr_initialize_ptr initializeCoreCLR = (coreclr_initialize_ptr)dlsym(coreclrLib, "coreclr_initialize");
        coreclr_execute_assembly_ptr executeAssembly = (coreclr_execute_assembly_ptr)dlsym(coreclrLib, "coreclr_execute_assembly");
        coreclr_shutdown_2_ptr shutdownCoreCLR = (coreclr_shutdown_2_ptr)dlsym(coreclrLib, "coreclr_shutdown_2");
        coreclr_create_delegate_ptr createDelegate = (coreclr_create_delegate_ptr)dlsym(coreclrLib, "coreclr_create_delegate");

        if (initializeCoreCLR == nullptr)
        {
            fprintf(stderr, "Function coreclr_initialize not found in the libcoreclr.so\n");
        }
        else if (executeAssembly == nullptr)
        {
            fprintf(stderr, "Function coreclr_execute_assembly not found in the libcoreclr.so\n");
        }
        else if (shutdownCoreCLR == nullptr)
        {
            fprintf(stderr, "Function coreclr_shutdown_2 not found in the libcoreclr.so\n");
        }
        else
        {
            // Check whether we are enabling server GC (off by default)
            const char* useServerGc = GetEnvValueBoolean(serverGcVar);

            // Check Globalization Invariant mode (false by default)
            const char* globalizationInvariant = GetEnvValueBoolean(globalizationInvariantVar);

            // Allowed property names:
            // APPBASE
            // - The base path of the application from which the exe and other assemblies will be loaded
            //
            // TRUSTED_PLATFORM_ASSEMBLIES
            // - The list of complete paths to each of the fully trusted assemblies
            //
            // APP_PATHS
            // - The list of paths which will be probed by the assembly loader
            //
            // APP_NI_PATHS
            // - The list of additional paths that the assembly loader will probe for ngen images
            //
            // NATIVE_DLL_SEARCH_DIRECTORIES
            // - The list of paths that will be probed for native DLLs called by PInvoke
            //
            const char *propertyKeys[] = {
                "TRUSTED_PLATFORM_ASSEMBLIES",
                "APP_PATHS",
                "APP_NI_PATHS",
                "NATIVE_DLL_SEARCH_DIRECTORIES",
                "System.GC.Server",
                "System.Globalization.Invariant",
            };
            const char *propertyValues[] = {
                // TRUSTED_PLATFORM_ASSEMBLIES
                tpaList.c_str(),
                // APP_PATHS
                appPath.c_str(),
                // APP_NI_PATHS
                appPath.c_str(),
                // NATIVE_DLL_SEARCH_DIRECTORIES
                nativeDllSearchDirs.c_str(),
                // System.GC.Server
                useServerGc,
                // System.Globalization.Invariant
                globalizationInvariant,
            };

            void* hostHandle;
            unsigned int domainId;

            int st = initializeCoreCLR(
                        currentExeAbsolutePath,
                        "unixcorerun",
                        sizeof(propertyKeys) / sizeof(propertyKeys[0]),
                        propertyKeys,
                        propertyValues,
                        &hostHandle,
                        &domainId);

            if (!SUCCEEDED(st))
            {
                fprintf(stderr, "coreclr_initialize failed - status: 0x%08x\n", st);
                exitCode = -1;
            }
            else
            {
                printf("executeAssembly...\n");
                st = executeAssembly(
                        hostHandle,
                        domainId,
                        managedAssemblyArgc,
                        managedAssemblyArgv,
                        managedAssemblyAbsolutePath,
                        (unsigned int*)&exitCode);

                /*
                typedef void (MainMethodFp)(const void* args);

                void *pfnDelegate = NULL;

                st = createDelegate(
                    hostHandle,
                    domainId,
                    "Assembly", // Target managed assembly
                    "ClassType", // Target managed type
                    "Main", // Target entry point (static method)
                    &pfnDelegate
                );
                ((MainMethodFp*)pfnDelegate)(NULL);
                */
                if (!SUCCEEDED(st))
                {
                    fprintf(stderr, "coreclr_execute_assembly failed - status: 0x%08x\n", st);
                    exitCode = -1;
                }

                int latchedExitCode = 0;
                st = shutdownCoreCLR(hostHandle, domainId, &latchedExitCode);
                if (!SUCCEEDED(st))
                {
                    fprintf(stderr, "coreclr_shutdown failed - status: 0x%08x\n", st);
                    exitCode = -1;
                }

                if (exitCode != -1)
                {
                    exitCode = latchedExitCode;
                }
            }
        }

        if (dlclose(coreclrLib) != 0)
        {
            fprintf(stderr, "Warning - dlclose failed\n");
        }
    }
    else
    {
        const char* error = dlerror();
        fprintf(stderr, "dlopen failed to open the libcoreclr.so with error %s\n", error);
    }

    return exitCode;
}


int m_DomainId = -1;
void * m_HostHandle = NULL;
coreclr_create_delegate_ptr createDelegate = NULL;

extern "C"
EXPORT int LoadManagedAssembly(
            const char* currentExeAbsolutePath,
            const char* clrFilesAbsolutePath,
            const char* managedAssemblyAbsolutePath,
            int managedAssemblyArgc,
            const char** managedAssemblyArgv)
{
    // Indicates failure
    int exitCode = -1;

#ifdef _ARM_
    // libunwind library is used to unwind stack frame, but libunwind for ARM
    // does not support ARM vfpv3/NEON registers in DWARF format correctly.
    // Therefore let's disable stack unwinding using DWARF information
    // See https://github.com/dotnet/coreclr/issues/6698
    //
    // libunwind use following methods to unwind stack frame.
    // UNW_ARM_METHOD_ALL          0xFF
    // UNW_ARM_METHOD_DWARF        0x01
    // UNW_ARM_METHOD_FRAME        0x02
    // UNW_ARM_METHOD_EXIDX        0x04
    putenv(const_cast<char *>("UNW_ARM_UNWIND_METHOD=6"));
#endif // _ARM_

    std::string coreClrDllPath(clrFilesAbsolutePath);
    coreClrDllPath.append("/");
    coreClrDllPath.append(coreClrDll);

    if (coreClrDllPath.length() >= PATH_MAX)
    {
        fprintf(stderr, "Absolute path to libcoreclr.so too long\n");
        return -1;
    }

    // Get just the path component of the managed assembly path
    std::string appPath;
    GetDirectory(managedAssemblyAbsolutePath, appPath);

    std::string tpaList;
    if (strlen(managedAssemblyAbsolutePath) > 0)
    {
        // Target assembly should be added to the tpa list. Otherwise corerun.exe
        // may find wrong assembly to execute.
        // Details can be found at https://github.com/dotnet/coreclr/issues/5631
        tpaList = managedAssemblyAbsolutePath;
        tpaList.append(":");
    }

    // Construct native search directory paths
    std::string nativeDllSearchDirs(appPath);
    char *coreLibraries = getenv("CORE_LIBRARIES");
    if (coreLibraries)
    {
        nativeDllSearchDirs.append(":");
        nativeDllSearchDirs.append(coreLibraries);
        if (std::strcmp(coreLibraries, clrFilesAbsolutePath) != 0)
        {
            AddFilesFromDirectoryToTpaList(coreLibraries, tpaList);
        }
    }

    nativeDllSearchDirs.append(":");
    nativeDllSearchDirs.append(clrFilesAbsolutePath);

    AddFilesFromDirectoryToTpaList(clrFilesAbsolutePath, tpaList);

    void* coreclrLib = dlopen(coreClrDllPath.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (coreclrLib != nullptr)
    {
        coreclr_initialize_ptr initializeCoreCLR = (coreclr_initialize_ptr)dlsym(coreclrLib, "coreclr_initialize");
        coreclr_execute_assembly_ptr executeAssembly = (coreclr_execute_assembly_ptr)dlsym(coreclrLib, "coreclr_execute_assembly");
        coreclr_shutdown_2_ptr shutdownCoreCLR = (coreclr_shutdown_2_ptr)dlsym(coreclrLib, "coreclr_shutdown_2");
        createDelegate = (coreclr_create_delegate_ptr)dlsym(coreclrLib, "coreclr_create_delegate");

        if (initializeCoreCLR == nullptr)
        {
            fprintf(stderr, "Function coreclr_initialize not found in the libcoreclr.so\n");
        }
        else if (executeAssembly == nullptr)
        {
            fprintf(stderr, "Function coreclr_execute_assembly not found in the libcoreclr.so\n");
        }
        else if (shutdownCoreCLR == nullptr)
        {
            fprintf(stderr, "Function coreclr_shutdown_2 not found in the libcoreclr.so\n");
        }
        else
        {
            // Check whether we are enabling server GC (off by default)
            const char* useServerGc = GetEnvValueBoolean(serverGcVar);

            // Check Globalization Invariant mode (false by default)
            const char* globalizationInvariant = GetEnvValueBoolean(globalizationInvariantVar);

            // Allowed property names:
            // APPBASE
            // - The base path of the application from which the exe and other assemblies will be loaded
            //
            // TRUSTED_PLATFORM_ASSEMBLIES
            // - The list of complete paths to each of the fully trusted assemblies
            //
            // APP_PATHS
            // - The list of paths which will be probed by the assembly loader
            //
            // APP_NI_PATHS
            // - The list of additional paths that the assembly loader will probe for ngen images
            //
            // NATIVE_DLL_SEARCH_DIRECTORIES
            // - The list of paths that will be probed for native DLLs called by PInvoke
            //
            const char *propertyKeys[] = {
                "TRUSTED_PLATFORM_ASSEMBLIES",
                "APP_PATHS",
                "APP_NI_PATHS",
                "NATIVE_DLL_SEARCH_DIRECTORIES",
                "System.GC.Server",
                "System.Globalization.Invariant",
            };
            const char *propertyValues[] = {
                // TRUSTED_PLATFORM_ASSEMBLIES
                tpaList.c_str(),
                // APP_PATHS
                appPath.c_str(),
                // APP_NI_PATHS
                appPath.c_str(),
                // NATIVE_DLL_SEARCH_DIRECTORIES
                nativeDllSearchDirs.c_str(),
                // System.GC.Server
                useServerGc,
                // System.Globalization.Invariant
                globalizationInvariant,
            };

            void* hostHandle;
            unsigned int domainId;

            int st = initializeCoreCLR(
                        currentExeAbsolutePath,
                        "unixcorerun",
                        sizeof(propertyKeys) / sizeof(propertyKeys[0]),
                        propertyKeys,
                        propertyValues,
                        &hostHandle,
                        &domainId);

            printf("initializeCoreCLR returned %d\n", st);
            if (!SUCCEEDED(st))
            {
                fprintf(stderr, "coreclr_initialize failed - status: 0x%08x\n", st);
                exitCode = -1;
            }
            else
            {
                m_HostHandle = hostHandle;
                m_DomainId = domainId;
                printf("Start CLR host with handle %p, domain id %d...\n", hostHandle, domainId);
                exitCode = 0;
            }
        }

        if (dlclose(coreclrLib) != 0)
        {
            fprintf(stderr, "Warning - dlclose failed\n");
        }
    }
    else
    {
        const char* error = dlerror();
        fprintf(stderr, "dlopen failed to open the libcoreclr.so with error %s\n", error);
    }

    return exitCode;
}

#define BYTE unsigned char

struct AssemblyFunctionCall
{
	char Assembly[256];
	char Class[256];
	char Function[256];
	BYTE Arguments[256];
};
#define ULONG       uint64_t
#define LONGLONG    int64_t
struct RemoteEntryInfo
{
	pid_t HostPID;
	const BYTE* UserData;
	ULONG UserDataSize;
};
void RtlLongLongToAsciiHex(LONGLONG InValue, char* InBuffer)
{
	ULONG           Index;
	ULONG           iChar;
	char            c;

	for (Index = 0, iChar = 0; Index < 64; Index += 4, iChar++)
	{
		c = ((LONGLONG)InValue >> Index) & 0x0F;

		if (c < 10)
			c += '0';
		else
			c += 'A' - 10;

		InBuffer[15 - iChar] = c;
	}

	InBuffer[16] = 0;
}
extern "C"
EXPORT int ExecuteManagedAssemblyClassFunction(AssemblyFunctionCall * args)
{
    // Indicates failure
    int exitCode = -1;
    int st = -1;
    char ParamString[17];
    RemoteEntryInfo EntryInfo;
    if(createDelegate != NULL) {
        printf("Executing .NET library assembly = %s, class = %s, function = %s...\n", args->Assembly, args->Class, args->Function);
        typedef void (MainMethodFp)(const void* args);

        void *pfnDelegate = NULL;

        while(m_HostHandle == NULL){
            sleep(1);
        }
        st = createDelegate(
            m_HostHandle,
            m_DomainId,
            args->Assembly, // Target managed assembly
            args->Class, // Target managed type
            args->Function, // Target entry point (static method)
            &pfnDelegate
        );
        exitCode = st;
        printf("createDelegate returend %d.\nCalling .NET delegate %p...\n", st, pfnDelegate);

        if (!SUCCEEDED(st))
        {
            fprintf(stderr, "coreclr_execute_assembly failed - status: 0x%08x\n", st);
            exitCode = -1;
        }
        else {
            LONGLONG argPtr = *(LONGLONG*)args->Arguments;
            if(argPtr != 0) {
              EntryInfo.UserData = (BYTE*)argPtr;
              EntryInfo.UserDataSize = 0x400;

              RtlLongLongToAsciiHex((LONGLONG)&EntryInfo, ParamString);
              ((MainMethodFp*)pfnDelegate)(ParamString);
            }
            else {
                ((MainMethodFp*)pfnDelegate)(NULL);
            }
        }
    }
    /*
    int latchedExitCode = 0;

    st = shutdownCoreCLR(hostHandle, domainId, &latchedExitCode);
    if (!SUCCEEDED(st))
    {
        fprintf(stderr, "coreclr_shutdown failed - status: 0x%08x\n", st);
        exitCode = -1;
    }


    if (exitCode != -1)
    {
        exitCode = latchedExitCode;
    }
    */

    return exitCode;
}

extern "C"
EXPORT int LoadManagedAssembly(
            const char* currentExeAbsolutePath,
            const char* clrFilesAbsolutePath,
            const char* managedAssemblyAbsolutePath,
            int managedAssemblyArgc,
            const char** managedAssemblyArgv);

extern "C" EXPORT void *LoadAssemblyBinaryArgs(BinaryLoaderArgs * args)
{
        char* clrFilesPath = args->CoreRootPath;
        char* managedAssemblyPath  = args->BinaryFilePath;

        if(args->CoreRootPath != NULL && args->BinaryFilePath != NULL) {
            printf("Managed assembly %s, CLR root %s\n", args->BinaryFilePath, args->CoreRootPath);
        }

        const char** managedAssemblyArgv = nullptr;
        int managedAssemblyArgc = 0;

        // Check if the specified managed assembly file exists
        struct stat sb;
        if (stat(managedAssemblyPath, &sb) == -1)
        {
            perror("Managed assembly not found");
            return NULL;
        }

        // Verify that the managed assembly path points to a file
        if (!S_ISREG(sb.st_mode))
        {
            fprintf(stderr, "The specified managed assembly is not a file\n");
            return NULL;
        }

        // Make sure we have a full path for argv[0].
        std::string argv0AbsolutePath;
        if (!GetEntrypointExecutableAbsolutePath(argv0AbsolutePath))
        {
            perror("Could not get full path");
            return NULL;
        }
        printf("%s\n", argv0AbsolutePath.c_str());

        std::string clrFilesAbsolutePath;
        if(!GetClrFilesAbsolutePath(argv0AbsolutePath.c_str(), clrFilesPath, clrFilesAbsolutePath))
        {
            return NULL;
        }
        printf("%s\n", clrFilesAbsolutePath.c_str());

        std::string managedAssemblyAbsolutePath;
        if (!GetAbsolutePath(managedAssemblyPath, managedAssemblyAbsolutePath))
        {
            perror("Failed to convert managed assembly path to absolute path");
            return NULL;
        }
        printf("%s\n", managedAssemblyAbsolutePath.c_str());

        printf("Calling LoadManagedAssembly...\n");

        int exitCode = LoadManagedAssembly(
                                argv0AbsolutePath.c_str(),
                                clrFilesAbsolutePath.c_str(),
                                managedAssemblyAbsolutePath.c_str(),
                                managedAssemblyArgc,
                                managedAssemblyArgv);
        printf("LoadManagedAssembly returned %d\n", exitCode);

        return NULL;
}
extern "C" EXPORT void* ExecuteDotnetAssembly(BinaryLoaderArgs * args)
{
        char* clrFilesPath = args->CoreRootPath;
        char* managedAssemblyPath  = args->BinaryFilePath;

        if(args->CoreRootPath != NULL && args->BinaryFilePath != NULL) {
            printf("Managed assembly %s, CLR root %s\n", args->BinaryFilePath, args->CoreRootPath);
        }

        const char** managedAssemblyArgv = nullptr;
        int managedAssemblyArgc = 0;

        // Check if the specified managed assembly file exists
        struct stat sb;
        if (stat(managedAssemblyPath, &sb) == -1)
        {
            perror("Managed assembly not found");
            return NULL;
        }

        // Verify that the managed assembly path points to a file
        if (!S_ISREG(sb.st_mode))
        {
            fprintf(stderr, "The specified managed assembly is not a file\n");
            return NULL;
        }

        // Make sure we have a full path for argv[0].
        std::string argv0AbsolutePath;
        if (!GetEntrypointExecutableAbsolutePath(argv0AbsolutePath))
        {
            perror("Could not get full path");
            return NULL;
        }
        printf("%s\n", argv0AbsolutePath.c_str());

        std::string clrFilesAbsolutePath;
        if(!GetClrFilesAbsolutePath(argv0AbsolutePath.c_str(), clrFilesPath, clrFilesAbsolutePath))
        {
            return NULL;
        }
        printf("%s\n", clrFilesAbsolutePath.c_str());

        std::string managedAssemblyAbsolutePath;
        if (!GetAbsolutePath(managedAssemblyPath, managedAssemblyAbsolutePath))
        {
            perror("Failed to convert managed assembly path to absolute path");
            return NULL;
        }
        printf("%s\n", managedAssemblyAbsolutePath.c_str());

        printf("Calling ExecuteManagedAssembly...\n");

        int exitCode = ExecuteManagedAssembly(
                                argv0AbsolutePath.c_str(),
                                clrFilesAbsolutePath.c_str(),
                                managedAssemblyAbsolutePath.c_str(),
                                managedAssemblyArgc,
                                managedAssemblyArgv);
        printf("ExecuteManagedAssembly returned %d\n", exitCode);

        return NULL;
}

__attribute__((destructor))
void ExitFunc()
{
  #if defined (__linux__)
      if(RemoteThreadMailbox != NULL)
      {
          free(RemoteThreadMailbox);
      }
  #endif
}
__attribute__((constructor))
void EntryPoint()
{
  printf("Loaded corerun dynamic library\n");
#if defined (__linux__)
  RemoteThreadMailbox = (RemoteThreadArgs*)malloc(sizeof(RemoteThreadArgs));
  memset(RemoteThreadMailbox, 0, sizeof(RemoteThreadArgs));

  pthread_t t;
  pthread_create(&t, NULL, CreateRemoteThread, NULL);
#endif
}
