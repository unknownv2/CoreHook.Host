#include "arguments.h"
#include "status_code.h"
#include "fx_muxer.h"
#include "corehost.h"

namespace coreload {
    coreclr::domain_id_t corehost::m_domain_id = 0;
    coreclr::host_handle_t corehost::m_handle = nullptr;

    int corehost::initialize_clr(
        arguments_t& arguments,
        const host_startup_info_t& host_info,
        host_mode_t mode)
    {
        return fx_muxer_t::initialize_clr(arguments, host_info, mode, corehost::m_domain_id, corehost::m_handle);
    }

    int corehost::create_delegate(
        const char* assembly_name,
        const char* type_name,
        const char* method_name,
        void** pfnDelegate)
    {
        assert(assembly_name != nullptr);
        assert(type_name != nullptr);
        assert(method_name != nullptr);
        assert(pfnDelegate != nullptr);

        auto hr = coreclr::create_delegate(
            corehost::m_handle,
            corehost::m_domain_id,
            assembly_name,
            type_name,
            method_name,
            reinterpret_cast<VOID**>(pfnDelegate));
        if (!SUCCEEDED(hr))
        {
            trace::error(_X("Failed to create delegate for managed library, HRESULT: 0x%X"), hr);
            return StatusCode::CoreClrExeFailure;
        }

        return StatusCode::Success;
    }

    int corehost::unload_runtime() {
        int exit_code = 0;

        auto hr = coreclr::shutdown(corehost::m_handle, corehost::m_domain_id, (int*)&exit_code);
        if (!SUCCEEDED(hr))
        {
            trace::warning(_X("Failed to shut down CoreCLR, HRESULT: 0x%X"), hr);
        }

        coreclr::unload();
        return exit_code;
    }
}