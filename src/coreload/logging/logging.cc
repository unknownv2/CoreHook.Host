#include "logging.h"

namespace coreload {

    namespace logging {

        void setup() {

        }

        void info(const pal::char_t* format, ...) {

        }

        void verbose(const pal::char_t* format, ...) {

        }

        void error(const pal::char_t* format, ...) {

        }


        void logger::log(
            const pal::char_t* severity,
            const pal::char_t* format,
            ...) {

            std::lock_guard<std::mutex> lock(mutex_log_);
            va_list args;
            va_start(args, format);
            pal::err_vprintf(format, args);
            va_end(args);
        }
        
    }
}
