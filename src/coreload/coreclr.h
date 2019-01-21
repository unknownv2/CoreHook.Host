#ifndef CLR_H
#define CLR_H
#include "pal.h"

namespace coreload {    
    namespace coreclr {
        typedef void* host_handle_t;
        typedef unsigned int domain_id_t;

        bool bind(const pal::string_t& libcoreclr_path);

        void unload();

        pal::hresult_t initialize(
            const char* exe_path,
            const char* app_domain_friendly_name,
            const char** property_keys,
            const char** property_values,
            int property_count,
            host_handle_t* host_handle,
            domain_id_t* domain_id);

        pal::hresult_t shutdown(
            host_handle_t host_handle,
            domain_id_t domain_id,
            int* latchedExitCode);

        pal::hresult_t execute_assembly(
            host_handle_t host_handle,
            domain_id_t domain_id,
            int argc,
            const char** argv,
            const char* managed_assembly_path,
            unsigned int* exit_code);

        pal::hresult_t create_delegate(
            host_handle_t host_handle,
            domain_id_t domain_id,
            const char* assembly_name,
            const char* type_name,
            const char* method_name,
            void** delegate);
    }
}
#endif // CLR_H
