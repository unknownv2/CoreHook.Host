#ifndef PAL_H
#define PAL_H

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cassert>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <algorithm>

#if defined(_WIN32)

#define NOMINMAX
#include <Windows.h>

#define LIB_PREFIX
#define MAKE_LIBNAME(NAME) (_X(NAME) _X(".dll"))

#define _X(s) L ## s

#define DIR_SEPARATOR L'\\'
#define PATH_SEPARATOR L';'
#define FALLBACK_HOST_RID _X("win10")

#else
#endif

#define LIBCLRJIT_NAME MAKE_LIBNAME("clrjit")

#define LIBCORECLR_FILENAME (LIB_PREFIX _X("coreclr"))
#define LIBCORECLR_NAME MAKE_LIBNAME("coreclr")

#define CORELIB_NAME _X("System.Private.CoreLib.dll")

#define LIBHOSTPOLICY_FILENAME (LIB_PREFIX _X("hostpolicy"))
#define LIBHOSTPOLICY_NAME MAKE_LIBNAME("hostpolicy")

namespace coreload {

    namespace pal {
        
#if defined(_WIN32)

#define STDMETHODCALLTYPE __stdcall

        typedef wchar_t char_t;
        typedef std::wstring string_t;
        typedef std::wstringstream stringstream_t;
        // TODO: Agree on the correct encoding of the files: The PoR for now is to
        // temporarily wchar for Windows and char for Unix. Current implementation
        // implicitly expects the contents on both Windows and Unix as char and
        // converts them to wchar in code for Windows. This line should become:
        // typedef std::basic_ifstream<pal::char_t> ifstream_t.
        typedef std::basic_ifstream<char> ifstream_t;
        typedef std::istreambuf_iterator<ifstream_t::char_type> istreambuf_iterator_t;
        typedef HRESULT hresult_t;
        typedef HMODULE dll_t;
        typedef FARPROC proc_t;

        inline string_t exe_suffix() { return _X(".exe"); }

        pal::string_t to_string(int value);

        bool getcwd(pal::string_t* recv);

        inline int cstrcasecmp(const char* str1, const char* str2) { return ::_stricmp(str1, str2); }
        inline int strcmp(const char_t* str1, const char_t* str2) { return ::wcscmp(str1, str2); }
        inline int strcasecmp(const char_t* str1, const char_t* str2) { return ::_wcsicmp(str1, str2); }
        inline int strncmp(const char_t* str1, const char_t* str2, int len) { return ::wcsncmp(str1, str2, len); }
        inline int strncasecmp(const char_t* str1, const char_t* str2, int len) { return ::_wcsnicmp(str1, str2, len); }

        pal::string_t to_lower(const pal::string_t& in);

        inline size_t strlen(const char_t* str) { return ::wcslen(str); }
        inline errno_t file_open(const pal::string_t& path, const char_t* mode, FILE* stream) { return ::_wfopen_s(&stream, path.c_str(), mode); }
        inline void file_vprintf(FILE* f, const char_t* format, va_list vl) { ::vfwprintf(f, format, vl); ::fputwc(_X('\n'), f); }
        inline void err_fputs(const char_t* message) { ::fputws(message, stderr); ::fputwc(_X('\n'), stderr); }
        inline void out_vprintf(const char_t* format, va_list vl) { ::vfwprintf(stdout, format, vl); ::fputwc(_X('\n'), stdout); }
        inline int str_vprintf(char_t* buffer, size_t count, size_t max_count, const char_t* format, va_list vl) { return ::_vsnwprintf_s(buffer, count, max_count, format, vl); }
        bool pal_utf8string(const pal::string_t& str, std::vector<char>* out);
        bool utf8_palstring(const std::string& str, pal::string_t* out);
        bool pal_clrstring(const pal::string_t& str, std::vector<char>* out);
        bool clr_palstring(const char* cstr, pal::string_t* out);
#else   
        
#endif
        pal::string_t get_timestamp();

        inline void file_flush(FILE *f) { std::fflush(f); }
        inline void err_flush() { std::fflush(stderr); }
        inline void out_flush() { std::fflush(stdout); }

        // Based upon https://github.com/dotnet/core-setup/blob/master/src/Microsoft.DotNet.PlatformAbstractions/Native/PlatformApis.cs
        pal::string_t get_current_os_rid_platform();
        inline pal::string_t get_current_os_fallback_rid()
        {
            pal::string_t fallbackRid(FALLBACK_HOST_RID);

            return fallbackRid;
        }

        bool touch_file(const pal::string_t& path);
        bool realpath(string_t* path, bool skip_error_logging = false);
        bool file_exists(const string_t& path);
        inline bool directory_exists(const string_t& path) { return file_exists(path); }
        void readdir(const string_t& path, const string_t& pattern, std::vector<pal::string_t>* list);
        void readdir(const string_t& path, std::vector<pal::string_t>* list);
        void readdir_onlydirectories(const string_t& path, const string_t& pattern, std::vector<pal::string_t>* list);
        void readdir_onlydirectories(const string_t& path, std::vector<pal::string_t>* list);

        bool get_own_executable_path(string_t* recv);
        bool getenv(const char_t* name, string_t* recv);
        bool get_default_servicing_directory(string_t* recv);

        //On Linux, there are no global locations
        //On Windows there will be only one global location
        bool get_global_dotnet_dirs(std::vector<pal::string_t>* recv);

        bool get_default_installation_dir(pal::string_t* recv);
        bool get_default_breadcrumb_store(string_t* recv);
        bool is_path_rooted(const string_t& path);

        int xtoi(const char_t* input);

        bool load_library(const string_t* path, dll_t* dll);
        proc_t get_symbol(dll_t library, const char* name);
        void unload_library(dll_t library);

        bool is_running_in_wow64();

        bool are_paths_equal_with_normalized_casing(const string_t& path1, const string_t& path2);
    }
}

#endif // PAL_H
