#include "pal.h"
#include "arguments.h"
#include "trace.h"
#include "deps_resolver.h"
#include "fx_muxer.h"
#include "utils.h"
#include "coreclr.h"
#include "cpprest/json.h"
#include "libhost.h"
#include "status_code.h"
#include "host_startup_info.h"
#include "hostpolicy.h"

namespace coreload {

    hostpolicy_init_t g_init;

    int corehost_load(host_interface_t* init)
    {
        trace::setup();

        // Re-initialize global state in case of re-entry
        g_init = hostpolicy_init_t();

        if (!hostpolicy_init_t::init(init, &g_init))
        {
            return StatusCode::LibHostInitFailure;
        }

        return 0;
    }

}