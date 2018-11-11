#ifndef PAL_H
#define PAL_H

#include <string>


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

namespace coreload {

    namespace pal {
        
#if defined(_WIN32)
        typedef wchar_t char_t;
        typedef std::wstring string_t;

        pal::string_t to_string(int value);
        pal::string_t to_lower(const pal::string_t& in);
        typedef HMODULE dll_t;
        typedef HRESULT hresult_t;
        typedef FARPROC proc_t;

        inline void err_vprintf(const char_t* format, va_list vl) { 
            ::vfwprintf(stderr, format, vl); ::fputwc(_X('\n'), stderr);
        }

#else
        
#endif

        proc_t get_symbol(dll_t library, const char* name);
        bool load_library(const string_t* path, dll_t* dll);
        bool realpath(string_t* path, bool skip_error_logging = false);
        void unload_library(dll_t library);
    }
}

#endif // PAL_H
