#ifndef LONG_FILE_SUPPORT_
#define LONG_FILE_SUPPORT_

#include "pal.h"

namespace coreload
{
    class LongFile
    {
    public:
        static const pal::string_t ExtendedPrefix;
        static const pal::string_t DevicePathPrefix;
        static const pal::string_t UNCPathPrefix;
        static const pal::string_t UNCExtendedPathPrefix;
        static const pal::char_t VolumeSeparatorChar;
        static const pal::char_t DirectorySeparatorChar;
        static const pal::char_t AltDirectorySeparatorChar;
    public:
        static bool IsExtended(const pal::string_t& path);
        static bool IsUNCExtended(const pal::string_t& path);
        static bool ContainsDirectorySeparator(const pal::string_t & path);
        static bool IsDirectorySeparator(const pal::char_t c);
        static bool IsPathNotFullyQualified(const pal::string_t& path);
        static bool IsDevice(const pal::string_t& path);
        static bool IsNormalized(const pal::string_t& path);
        static bool ShouldNormalize(const pal::string_t& path);
    };

} // namespace coreload

#endif // LONG_FILE_SUPPORT_