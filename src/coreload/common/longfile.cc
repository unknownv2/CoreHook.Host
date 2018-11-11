
#include "longfile.h"

namespace coreload {

    bool LongFile::IsPathNotFullyQualified(const pal::string_t& path) {

        if (path.length() < 2) {
            return true;
        }
        if (IsDirectorySeparator(path[0])) {
            return !IsDirectorySeparator(path[1]);
        }
        return !((path.length() >= 3))
            && (path[1] == VolumeSeparatorChar)
            && IsDirectorySeparator(path[2]);
    }

    bool LongFile::IsDirectorySeparator(const pal::char_t c) {
        return c == DirectorySeparatorChar || c == AltDirectorySeparatorChar;
    }

    bool LongFile::IsNormalized(const pal::string_t& path) {
        return path.empty() 
            || LongFile::IsDevice(path)
            || LongFile::IsExtended(path)
            || LongFile::IsUNCExtended(path);
    }

    bool LongFile::IsDevice(const pal::string_t& path) {
        return path.compare(0, DevicePathPrefix.length(), DevicePathPrefix) == 0;
    }

    bool LongFile::IsExtended(const pal::string_t & path) {
        return path.compare(0, ExtendedPrefix.length(), ExtendedPrefix) == 0;
    }

    bool LongFile::IsUNCExtended(const pal::string_t& path) {
        return path.compare(0, UNCExtendedPathPrefix.length(), UNCExtendedPathPrefix) == 0;
    }
}