#include "pal.h"
#include "logging.h"
#include "longfile.h"

#include <cassert>

namespace coreload {

    pal::proc_t pal::get_symbol(dll_t library, const char* name) {
        auto result = ::GetProcAddress(library, name);
        if (result == nullptr) {
            logging::info(_X("Probed for and did not resolve library symbol %s"), name);
        }
        return result;
    }

    bool pal::load_library(const string_t* in_path, dll_t* dll) {

        string_t path = *in_path;

        if (LongFile::IsPathNotFullyQualified(path)) {
            if (!pal::realpath(&path)) {

            }
        }
        

        return true;
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


}