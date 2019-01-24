#include "arguments.h"

namespace coreload
{
    arguments_t::arguments_t() :
        managed_application(_X("")),
        host_path(_X("")),
        app_root(_X("")),
        app_argc(0),
        app_argv(nullptr),
        core_servicing(_X("")),
        deps_path(_X(""))
    {

    }
} // namespace coreload