set(TARGET coreload)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    common
    logging
    json/casablanca/include
)

set(SOURCES
    json/casablanca/src/json/json.cpp
    json/casablanca/src/json/json_parsing.cpp
    json/casablanca/src/json/json_serialization.cpp
    json/casablanca/src/utilities/asyncrt_utils.cpp
    common/longfile.cc
    common/pal.windows.cc
    common/trace.cc
    common/utils.cc
    arguments.cc
    coreclr.cc
    corehost.cc
    deps_entry.cc
    deps_format.cc
    deps_resolver.cc
    framework_info.cc
    fx_definition.cc
    fx_muxer.cc
    fx_muxer.messages.cc
    fx_reference.cc
    fx_ver.cc
    host_startup_info.cc
    libhost.cc
    runtime_config.cc
    version.cc
)

add_library(coreload STATIC ${SOURCES})

add_subdirectory(dll)