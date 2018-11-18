// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef TRACE_H
#define TRACE_H

#include "pal.h"

namespace coreload {
    namespace trace
    {
        void setup();
        void enable();
        bool is_enabled();
        void verbose(const pal::char_t* format, ...);
        void info(const pal::char_t* format, ...);
        void warning(const pal::char_t* format, ...);
        void error(const pal::char_t* format, ...);
        void println(const pal::char_t* format, ...);
        void println();
        void flush();
    };
}
#endif // TRACE_H