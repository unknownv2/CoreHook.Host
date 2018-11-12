#ifndef FX_DEFINITION_H
#define FX_DEFINITION_H

#include "pal.h"
#include "deps_format.h"
#include "runtime_config.h"

namespace coreload {

    class FxDefinition {
    public:
        FxDefinition();

        FxDefinition(
            const pal::string_t& name,
            const pal::string_t& dir,
            const pal::string_t& requested_version,
            const pal::string_t& found_version);

    };
    typedef std::vector<std::unique_ptr<FxDefinition>> fx_definition_vector_t;
}

#endif // FX_DEFINITION_H