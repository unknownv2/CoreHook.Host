#ifndef PAL_H
#define PAL_H

#include <string>

namespace coreload {

    namespace pal {
        
        typedef wchar_t char_t;
        typedef std::wstring string;

        pal::string to_string(int value);
        pal::string to_lower(const pal::string& in);

    }
}

#endif