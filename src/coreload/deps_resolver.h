#ifndef DEPS_RESOLVER_H
#define DEPS_RESOLVER_H

#include <vector>

#include "pal.h"

namespace coreload {

    struct ProbePaths {
        pal::string_t Tpa;
        pal::string_t Native;
        pal::string_t Resources;
        pal::string_t CoreClr;
        pal::string_t ClrJit;
    };

    class DepsResolver {

    };
}

#endif // DEPS_RESOLVER_H
