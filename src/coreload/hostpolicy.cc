// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

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
//#include "breadcrumbs.h"
#include "host_startup_info.h"
#include "hostpolicy.h"
namespace coreload {
#define HOST_POLICY_PKG_REL_DIR "runtimes/win-x64/native"
#define HOST_POLICY_PKG_NAME "runtimes.win-x64.Microsoft.NETCore.DotNetHostPolicy"
#define HOST_POLICY_PKG_VER "3.0.0-preview1-26822-0"
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