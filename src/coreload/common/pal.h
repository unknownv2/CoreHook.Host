#ifndef PAL_H
#define PAL_H

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cassert>
#include <vector>

#if defined(_WIN32)

#define NOMINMAX
#include <Windows.h>

#define LIB_PREFIX
#define MAKE_LIBNAME(NAME) (_X(NAME) _X(".dll"))

#define _X(s) L ## s

#define DIR_SEPARATOR L'\\'
#define PATH_SEPARATOR L';'

#else
#endif

#define LIBCORECLR_FILENAME (LIB_PREFIX _X("coreclr"))
#define LIBCORECLR_NAME MAKE_LIBNAME("coreclr")

#define LIBHOSTPOLICY_FILENAME (LIB_PREFIX _X("hostpolicy"))
#define LIBHOSTPOLICY_NAME MAKE_LIBNAME("hostpolicy")

namespace coreload {

    namespace pal {
        
#if defined(_WIN32)
        typedef wchar_t char_t;
        typedef std::wstring string_t;
        typedef std::wstringstream stringstream_t;

        pal::string_t to_string(int value);
        pal::string_t to_lower(const pal::string_t& in);
        typedef HMODULE dll_t;
        typedef HRESULT hresult_t;
        typedef FARPROC proc_t;

        inline void err_vprintf(const char_t* format, va_list vl) { 
            ::vfwprintf(stderr, format, vl); ::fputwc(_X('\n'), stderr);
        }
        bool get_own_executable_path(string_t* recv);
        inline string_t exe_suffix() { return _X(".exe"); }
        inline int cstrcasecmp(const char* str1, const char* str2) { return ::_stricmp(str1, str2); }
        inline int strcmp(const char_t* str1, const char_t* str2) { return ::wcscmp(str1, str2); }
        inline int strcasecmp(const char_t* str1, const char_t* str2) { return ::_wcsicmp(str1, str2); }
        inline int strncmp(const char_t* str1, const char_t* str2, int len) { return ::wcsncmp(str1, str2, len); }
        inline int strncasecmp(const char_t* str1, const char_t* str2, int len) { return ::_wcsnicmp(str1, str2, len); }
#else   
        
#endif
        bool pal_utf8string(const pal::string_t& str, std::vector<char>* out);
        bool pal_clrstring(const pal::string_t& str, std::vector<char>* out);
        proc_t get_symbol(dll_t library, const char* name);
        bool load_library(const string_t* path, dll_t* dll);
        bool realpath(string_t* path, bool skip_error_logging = false);
        void unload_library(dll_t library);
        bool file_exists(const string_t& path);
        bool is_path_rooted(const string_t& path);
    }
}

#endif // PAL_H
