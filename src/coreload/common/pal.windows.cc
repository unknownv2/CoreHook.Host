#include "pal.h"
#include "logging.h"
#include "longfile.h"
#include "path_utils.h"
#include <cassert>

namespace coreload {

    bool GetModuleFileNameWrapper(HMODULE hModule, pal::string_t* recv) {
        pal::string_t path;
        DWORD dwModuleFileName = MAX_PATH / 2;

        do {
            path.resize(dwModuleFileName * 2);
            dwModuleFileName = GetModuleFileNameW(hModule, (LPWSTR)path.data(), path.size());
        } while (dwModuleFileName == path.size());

        if (dwModuleFileName != 0) {
            *recv = path;
            return true;
        }

        return false;
    }

    pal::proc_t pal::get_symbol(dll_t library, const char* name) {
        auto result = ::GetProcAddress(library, name);
        if (result == nullptr) {
            coreload::logging::logger::instance().error(
                _X("Probed for and did not resolve library symbol %s"), name);
        }
        return result;
    }

    bool pal::load_library(const string_t* in_path, dll_t* dll) {
        string_t path = *in_path;

        if (LongFile::IsPathNotFullyQualified(path)) {
            if (!pal::realpath(&path)) {
                logging::error(_X("Failed to load the dll from [%s], HRESULT: 0x%X"), path.c_str(), HRESULT_FROM_WIN32(GetLastError()));
                return false;
            }
        }

        //Adding the assert to ensure relative paths which are not just filenames are not used for LoadLibrary Calls
        assert(!LongFile::IsPathNotFullyQualified(path) || !LongFile::ContainsDirectorySeparator(path));

        *dll = ::LoadLibraryExW(path.c_str(), NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        if (*dll == nullptr)
        {
            logging::error(_X("Failed to load the dll from [%s], HRESULT: 0x%X"), path.c_str(), HRESULT_FROM_WIN32(GetLastError()));
            return false;
        }

        // Pin the module
        HMODULE dummy_module;
        if (!::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, path.c_str(), &dummy_module))
        {
            logging::error(_X("Failed to pin library [%s] in [%s]"), path.c_str(), _STRINGIFY(__FUNCTION__));
            return false;
        }

        if (logging::is_enabled())
        {
            string_t buf;
            GetModuleFileNameWrapper(*dll, &buf);
            logging::info(_X("Loaded library from %s"), buf.c_str());
        }

        return true;

        return true;
    }

    void pal::unload_library(dll_t library) {
        // No-op. On windows, we pin the library, so it can't be unloaded.
    }

    bool pal::realpath(pal::string_t* path, bool skip_error_logging) {
        if (LongFile::IsNormalized(path->c_str())) {
            WIN32_FILE_ATTRIBUTE_DATA data;
            if (GetFileAttributesExW(path->c_str(), GetFileExInfoStandard, &data) != 0) {
                return true;
            }
        }

        char_t buf[MAX_PATH];
        size_t size = ::GetFullPathNameW(path->c_str(), MAX_PATH, buf, nullptr);
        if (size == 0) {
            if (!skip_error_logging) {
                logging::error(_X("Error resolving full path [%s]"), path->c_str());
            }
            return false;
        }

        string_t str;
        if (size < MAX_PATH) {
            str.assign(buf);
        }
        else {
            str.resize(size + LongFile::UNCExtendedPathPrefix.length(), 0);

            size = ::GetFullPathNameW(path->c_str(), (DWORD)size, (LPWSTR)str.data(), nullptr);
            assert(size <= str.size());

            if (size == 0) {
                if (!skip_error_logging) {
                    logging::error(_X("Error resolving full path [%s]"), path->c_str());
                }
                return false;
            }

            const string_t* prefix = &LongFile::ExtendedPrefix;
            //Check if the resolved path is a UNC. By default we assume relative path to resolve to disk 
            if (str.compare(0, LongFile::UNCPathPrefix.length(), LongFile::UNCPathPrefix) == 0) {
                prefix = &LongFile::UNCExtendedPathPrefix;
                str.erase(0, LongFile::UNCPathPrefix.length());
                size = size - LongFile::UNCPathPrefix.length();
            }

            str.insert(0, *prefix);
            str.resize(size + prefix->length());
            str.shrink_to_fit();
        }

        WIN32_FILE_ATTRIBUTE_DATA data;
        if (GetFileAttributesExW(str.c_str(), GetFileExInfoStandard, &data) != 0) {
            *path = str;
            return true;
        }

        return false;
    }

    bool pal::file_exists(const string_t& path)
    {
        if (path.empty()) {
            return false;
        }

        string_t tmp(path);
        return pal::realpath(&tmp, true);
    }

    bool pal::is_path_rooted(const string_t& path) {
        return path.length() >= 2 && path[1] == L':';
    }
}
