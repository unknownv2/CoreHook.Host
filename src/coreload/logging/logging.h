#ifndef LOGGING_H
#define LOGGING_H

#include "pal.h"
#include <mutex>

namespace coreload {

    namespace logging {

        void setup();
        void info(const pal::char_t* format, ...);
        void verbose(const pal::char_t* format, ...);
        void error(const pal::char_t* format, ...);
        bool is_enabled();
        class logger {
        public:
            static logger& instance() {
                static logger inst;
                return inst;
            }

            template <typename... Args>
            void verbose(const pal::char_t* format, Args... args) {
                log(_X("VERBOSE"), format, args...);
            }

            template <typename... Args>
            void trace(const pal::char_t* format, Args... args) {
                log(_X("TRACE"), format, args...);
            }
            template <typename... Args>
            void error(const pal::char_t* format, Args... args) {
                log(_X("ERROR"), format, args...);
            }

        private:
            void log(
                const pal::char_t* severity,
                const pal::char_t* format,
                ...); 

            std::mutex mutex_log_;

        };
    }
}

#endif // LOGGING_H

