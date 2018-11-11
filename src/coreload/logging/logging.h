#ifndef LOGGING_H
#define LOGGING_H

#include "pal.h"

namespace coreload {

    namespace logging {

        void setup();
        void info(const pal::char_t* format, ...);
        void verbose(const pal::char_t* format, ...);
        void error(const pal::char_t* format, ...);

        class logger {
        public:
            static logger& instance() {
                static logger inst;
                return inst;
            }
        };
    }
}

#endif // LOGGING_H

