
#ifndef _LONG_FILE_SUPPORT
#define _LONG_FILE_SUPPORT

#include "pal.h"

namespace coreload {
    class LongFile {
    public:
        static const pal::string_t ExtendedPrefix;
        static const pal::string_t DevicePathPrefix;
        static const pal::string_t UNCPathPrefix;
        static const pal::string_t UNCExtendedPathPrefix;
        static const pal::char_t VolumeSeparatorChar;
        static const pal::char_t DirectorySeparatorChar;
        static const pal::char_t AltDirectorySeparatorChar;

        static bool IsPathNotFullyQualified(const pal::string_t& path);
        static bool IsDirectorySeparator(const pal::char_t c);
        static bool IsNormalized(const pal::string_t& path);
        static bool IsDevice(const pal::string_t& path);
        static bool IsExtended(const pal::string_t& path);
        static bool IsUNCExtended(const pal::string_t& path);
    };
}

#endif // _LONG_FILE_SUPPORT