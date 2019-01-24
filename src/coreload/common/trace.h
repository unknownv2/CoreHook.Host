#ifndef TRACE_H_
#define TRACE_H_

#include "pal.h"

namespace coreload
{
    namespace trace
    {
        void setup();
        bool enable();
        bool is_enabled();
        void verbose(const pal::char_t* format, ...);
        void info(const pal::char_t* format, ...);
        void warning(const pal::char_t* format, ...);
        void error(const pal::char_t* format, ...);
        void println(const pal::char_t* format, ...);
        void println();
        void flush();

        typedef void(*error_writer_fn)(const pal::char_t* message);

        // Sets a callback which is called whenever error is to be written
        // The setting is per-thread (thread local). If no error writer is set for a given thread
        // the error is written to stderr.
        // The callback is set for the current thread which calls this function.
        // The function returns the previously registered writer for the current thread (or null)
        error_writer_fn set_error_writer(error_writer_fn error_writer);

        // Returns the currently set callback for error writing
        error_writer_fn get_error_writer();
    };

} // namespace coreload

#endif // TRACE_H_
