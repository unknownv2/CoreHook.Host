#pragma once
#ifndef COREHOST_H
#define COREHOST_H

#include "libhost.h"
#include "coreclr.h"

namespace coreload {

    class corehost {
    public:
        static coreclr::host_handle_t m_handle;
        static coreclr::domain_id_t m_domain_id;
        static int initialize_clr(
            arguments_t& arguments,
            const host_startup_info_t& host_info,
            host_mode_t mode);

        static int create_delegate(
            const char* assembly,
            const char* type,
            const char* method_name,
            void** pfnDelegate);

        static int unload_runtime();
    };
}

#endif // COREHOST_H
