#include "arguments.h"
#include "path_utils.h"
#include "coreclr.h"

namespace coreload {

    Arguments::Arguments() :
        managed_application(_X("")),
        host_path(_X("")),
        app_root(_X("")),
        app_argc(0),
        app_argv(nullptr),
        core_servicing(_X("")),
        deps_path(_X("")) {

    }
}