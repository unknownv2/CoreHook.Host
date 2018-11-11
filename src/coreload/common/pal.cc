#include "pal.h"

namespace coreload {

    namespace pal {

        pal::string_t to_lower(const coreload::pal::string_t& in)
        {
            return nullptr;
        }

        pal::string_t to_string(int value)
        {
            return std::to_wstring(value);
        }
    }
}